// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Vertexing/ImpactPointEstimator.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Propagator/PropagatorOptions.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Utilities/AngleHelpers.hpp"
#include "Acts/Utilities/MathHelpers.hpp"
#include "Acts/Vertexing/VertexingError.hpp"

namespace Acts {

namespace {
template <typename vector_t>
Result<double> getVertexCompatibilityImpl(const GeometryContext& gctx,
                                          const BoundTrackParameters* trkParams,
                                          const vector_t& vertexPos) {
  static constexpr int nDim = vector_t::RowsAtCompileTime;
  static_assert(nDim == 3 || nDim == 4,
                "The number of dimensions nDim must be either 3 or 4.");

  static_assert(vector_t::RowsAtCompileTime == nDim,
                "The dimension of the vertex position vector must match nDim.");

  if (trkParams == nullptr) {
    return VertexingError::EmptyInput;
  }

  // Retrieve weight matrix of the track's local x-, y-, and time-coordinate
  // (the latter only if nDim = 4). For this, the covariance needs to be set.
  if (!trkParams->covariance().has_value()) {
    return VertexingError::NoCovariance;
  }
  ActsSquareMatrix<nDim - 1> subCovMat;
  if constexpr (nDim == 3) {
    subCovMat = trkParams->spatialImpactParameterCovariance().value();
  } else {
    subCovMat = trkParams->impactParameterCovariance().value();
  }
  ActsSquareMatrix<nDim - 1> weight = subCovMat.inverse();

  // Orientation of the surface (i.e., axes of the corresponding coordinate
  // system)
  RotationMatrix3 surfaceAxes =
      trkParams->referenceSurface().transform(gctx).rotation();
  // Origin of the surface coordinate system
  Vector3 surfaceOrigin =
      trkParams->referenceSurface().transform(gctx).translation();

  // x- and y-axis of the surface coordinate system
  Vector3 xAxis = surfaceAxes.col(0);
  Vector3 yAxis = surfaceAxes.col(1);

  // Vector pointing from the surface origin to the vertex position
  // TODO: The vertex should always be at the surfaceOrigin since the
  // track parameters should be obtained by estimate3DImpactParameters.
  // Therefore, originToVertex should always be 0, which is currently not the
  // case.
  Vector3 originToVertex = vertexPos.template head<3>() - surfaceOrigin;

  // x-, y-, and possibly time-coordinate of the vertex and the track in the
  // surface coordinate system
  ActsVector<nDim - 1> localVertexCoords;
  localVertexCoords.template head<2>() =
      Vector2(originToVertex.dot(xAxis), originToVertex.dot(yAxis));

  ActsVector<nDim - 1> localTrackCoords;
  localTrackCoords.template head<2>() =
      Vector2(trkParams->parameters()[eX], trkParams->parameters()[eY]);

  // Fill time coordinates if we check the 4D vertex compatibility
  if constexpr (nDim == 4) {
    localVertexCoords(2) = vertexPos(3);
    localTrackCoords(2) = trkParams->parameters()[eBoundTime];
  }

  // residual
  ActsVector<nDim - 1> residual = localTrackCoords - localVertexCoords;

  // return chi2
  return residual.dot(weight * residual);
}

/// @brief Performs a Newton approximation to retrieve a point
/// of closest approach in 3D to a reference position
///
/// @param helixCenter Position of the helix center
/// @param vtxPos Vertex position
/// @param phi Azimuthal momentum angle
/// @note Modifying phi corresponds to moving along the track. This function
/// optimizes phi until we reach a 3D PCA.
/// @param theta Polar momentum angle (constant along the track)
/// @param rho Signed helix radius
///
/// @return Phi value at 3D PCA
Result<double> performNewtonOptimization(
    const Vector3& helixCenter, const Vector3& vtxPos, double phi, double theta,
    double rho, const ImpactPointEstimator::Config& cfg, const Logger& logger) {
  double sinPhi = std::sin(phi);
  double cosPhi = std::cos(phi);

  int nIter = 0;
  bool hasConverged = false;

  double cotTheta = 1. / std::tan(theta);

  double xO = helixCenter.x();
  double yO = helixCenter.y();
  double zO = helixCenter.z();

  double xVtx = vtxPos.x();
  double yVtx = vtxPos.y();
  double zVtx = vtxPos.z();

  // Iterate until convergence is reached or the maximum amount of iterations
  // is exceeded
  while (!hasConverged && nIter < cfg.maxIterations) {
    double derivative = rho * ((xVtx - xO) * cosPhi + (yVtx - yO) * sinPhi +
                               (zVtx - zO + rho * phi * cotTheta) * cotTheta);
    double secDerivative = rho * (-(xVtx - xO) * sinPhi + (yVtx - yO) * cosPhi +
                                  rho * cotTheta * cotTheta);

    if (secDerivative < 0.) {
      ACTS_ERROR(
          "Encountered negative second derivative during Newton "
          "optimization.");
      return VertexingError::NumericFailure;
    }

    double deltaPhi = -derivative / secDerivative;

    phi += deltaPhi;
    sinPhi = std::sin(phi);
    cosPhi = std::cos(phi);

    nIter += 1;

    if (std::abs(deltaPhi) < cfg.precision) {
      hasConverged = true;
    }
  }  // end while loop

  if (!hasConverged) {
    ACTS_ERROR("Newton optimization did not converge.");
    return VertexingError::NotConverged;
  }
  return phi;
}

// Note: always return Vector4, we'll chop off the last component if needed
template <typename vector_t>
Result<std::pair<Vector4, Vector3>> getDistanceAndMomentumImpl(
    const GeometryContext& gctx, const BoundTrackParameters& trkParams,
    const vector_t& vtxPos, const ImpactPointEstimator::Config& cfg,
    ImpactPointEstimator::State& state, const Logger& logger) {
  static constexpr int nDim = vector_t::RowsAtCompileTime;
  static_assert(nDim == 3 || nDim == 4,
                "The number of dimensions nDim must be either 3 or 4.");

  // Reference point R
  Vector3 refPoint = trkParams.referenceSurface().center(gctx);

  // Extract charge-related particle parameters
  double absoluteCharge = trkParams.particleHypothesis().absoluteCharge();
  double qOvP = trkParams.parameters()[BoundIndices::eBoundQOverP];

  // Z-component of the B field at the reference position.
  // Note that we assume a constant B field here!
  auto fieldRes = cfg.bField->getField(refPoint, state.fieldCache);
  if (!fieldRes.ok()) {
    ACTS_ERROR("In getDistanceAndMomentum, the B field at\n"
               << refPoint << "\ncould not be retrieved.");
    return fieldRes.error();
  }
  double bZ = (*fieldRes)[eZ];

  // The particle moves on a straight trajectory if its charge is 0 or if there
  // is no B field. In that case, the 3D PCA can be calculated analytically, see
  // Sec 3.2 of the reference.
  if (absoluteCharge == 0. || bZ == 0.) {
    // Momentum direction (constant for straight tracks)
    Vector3 momDirStraightTrack = trkParams.direction();

    // Current position on the track
    Vector3 positionOnTrack = trkParams.position(gctx);

    // Distance between positionOnTrack and the 3D PCA
    double distanceToPca =
        (vtxPos.template head<3>() - positionOnTrack).dot(momDirStraightTrack);

    // 3D PCA
    ActsVector<nDim> pcaStraightTrack;
    pcaStraightTrack.template head<3>() =
        positionOnTrack + distanceToPca * momDirStraightTrack;
    if constexpr (nDim == 4) {
      // Track time at positionOnTrack
      double timeOnTrack = trkParams.parameters()[BoundIndices::eBoundTime];

      double m0 = trkParams.particleHypothesis().mass();
      double p = trkParams.particleHypothesis().extractMomentum(qOvP);

      // Speed in units of c
      double beta = p / fastHypot(p, m0);

      pcaStraightTrack[3] = timeOnTrack + distanceToPca / beta;
    }

    // Vector pointing from the vertex position to the 3D PCA
    Vector4 deltaRStraightTrack{Vector4::Zero()};
    deltaRStraightTrack.head<nDim>() = pcaStraightTrack - vtxPos;

    return std::pair(deltaRStraightTrack, momDirStraightTrack);
  }

  // Charged particles in a constant B field follow a helical trajectory. In
  // that case, we calculate the 3D PCA using the Newton method, see Sec 4.2 in
  // the reference.

  // Spatial Perigee parameters (i.e., spatial parameters of 2D PCA)
  double d0 = trkParams.parameters()[BoundIndices::eBoundLoc0];
  double z0 = trkParams.parameters()[BoundIndices::eBoundLoc1];
  // Momentum angles at 2D PCA
  double phiP = trkParams.parameters()[BoundIndices::eBoundPhi];
  double theta = trkParams.parameters()[BoundIndices::eBoundTheta];
  // Functions of the polar angle theta for later use
  double sinTheta = std::sin(theta);
  double cotTheta = 1. / std::tan(theta);

  // Set optimization variable phi to the angle at the 2D PCA as a first guess.
  // Note that phi corresponds to phiV in the reference.
  double phi = phiP;

  // Signed radius of the helix on which the particle moves
  double rho = sinTheta * (1. / qOvP) / bZ;

  // Position of the helix center.
  // We can set the z-position to a convenient value since it is not fixed by
  // the Perigee parameters. Note that phi = phiP because we did not start the
  // optimization yet.
  Vector3 helixCenter =
      refPoint + Vector3(-(d0 - rho) * std::sin(phi),
                         (d0 - rho) * std::cos(phi), z0 + rho * phi * cotTheta);

  // Use Newton optimization method to iteratively change phi until we arrive at
  // the 3D PCA
  auto res = performNewtonOptimization(helixCenter, vtxPos.template head<3>(),
                                       phi, theta, rho, cfg, logger);
  if (!res.ok()) {
    return res.error();
  }
  // Set new phi value
  phi = *res;

  double cosPhi = std::cos(phi);
  double sinPhi = std::sin(phi);

  // Momentum direction at the 3D PCA.
  // Note that we have thetaV = thetaP = theta since the polar angle does not
  // change in a constant B field.
  Vector3 momDir =
      Vector3(cosPhi * sinTheta, sinPhi * sinTheta, std::cos(theta));

  // 3D PCA (point P' in the reference). Note that the prefix "3D" does not
  // refer to the dimension of the pca variable. Rather, it indicates that we
  // minimized the 3D distance between the track and the reference point.
  ActsVector<nDim> pca;
  pca.template head<3>() =
      helixCenter + rho * Vector3(-sinPhi, cosPhi, -cotTheta * phi);

  if constexpr (nDim == 4) {
    // Time at the 2D PCA P
    double tP = trkParams.parameters()[BoundIndices::eBoundTime];

    double m0 = trkParams.particleHypothesis().mass();
    double p = trkParams.particleHypothesis().extractMomentum(qOvP);

    // Speed in units of c
    double beta = p / fastHypot(p, m0);

    pca[3] = tP - rho / (beta * sinTheta) * (phi - phiP);
  }
  // Vector pointing from the vertex position to the 3D PCA
  Vector4 deltaR{Vector4::Zero()};
  deltaR.head<nDim>() = pca - vtxPos;

  return std::pair(deltaR, momDir);
}

}  // namespace

Result<double> ImpactPointEstimator::calculateDistance(
    const GeometryContext& gctx, const BoundTrackParameters& trkParams,
    const Vector3& vtxPos, State& state) const {
  auto res = getDistanceAndMomentumImpl(gctx, trkParams, vtxPos, m_cfg, state,
                                        *m_logger);

  if (!res.ok()) {
    return res.error();
  }

  // Return distance (we get a 4D vector in all cases, but we only need the
  // position norm)
  return res.value().first.template head<3>().norm();
}

Result<BoundTrackParameters> ImpactPointEstimator::estimate3DImpactParameters(
    const GeometryContext& gctx, const MagneticFieldContext& mctx,
    const BoundTrackParameters& trkParams, const Vector3& vtxPos,
    State& state) const {
  auto res = getDistanceAndMomentumImpl(gctx, trkParams, vtxPos, m_cfg, state,
                                        *m_logger);

  if (!res.ok()) {
    return res.error();
  }

  // Vector pointing from vertex to 3D PCA
  Vector3 deltaR = res.value().first.head<3>();

  // Get corresponding unit vector
  deltaR.normalize();

  // Momentum direction at vtxPos
  Vector3 momDir = res.value().second;

  // To understand why deltaR and momDir are not orthogonal, let us look at the
  // x-y-plane. Since we computed the 3D PCA, the 2D distance between the vertex
  // and the PCA is not necessarily minimal (see Fig. 4.2 in the reference). As
  // a consequence, the momentum and the vector connecting the vertex and the
  // PCA are not orthogonal to each other.
  Vector3 orthogonalDeltaR = deltaR - (deltaR.dot(momDir)) * momDir;

  // Vector perpendicular to momDir and orthogonalDeltaR
  Vector3 perpDir = momDir.cross(orthogonalDeltaR);

  // Cartesian coordinate system with:
  // -) origin at the vertex position
  // -) z-axis in momentum direction
  // -) x-axis approximately in direction of the 3D PCA (slight deviations
  // because it was modified to make if orthogonal to momDir)
  // -) y-axis is calculated to be orthogonal to x- and z-axis
  // The transformation is represented by a 4x4 matrix with 0 0 0 1 in the last
  // row.
  Transform3 coordinateSystem;
  // First three columns correspond to coordinate system axes
  coordinateSystem.matrix().block<3, 1>(0, 0) = orthogonalDeltaR;
  coordinateSystem.matrix().block<3, 1>(0, 1) = perpDir;
  coordinateSystem.matrix().block<3, 1>(0, 2) = momDir;
  // Fourth column corresponds to origin of the coordinate system
  coordinateSystem.matrix().block<3, 1>(0, 3) = vtxPos;

  // Surface with normal vector in direction of the z axis of coordinateSystem
  std::shared_ptr<PlaneSurface> planeSurface =
      Surface::makeShared<PlaneSurface>(coordinateSystem);

  auto intersection =
      planeSurface
          ->intersect(gctx, trkParams.position(gctx), trkParams.direction(),
                      BoundaryTolerance::Infinite())
          .closest();

  // Create propagator options
  PropagatorPlainOptions pOptions(gctx, mctx);
  pOptions.direction =
      Direction::fromScalarZeroAsPositive(intersection.pathLength());

  // Propagate to the surface; intersection corresponds to an estimate of the 3D
  // PCA. If deltaR and momDir were orthogonal the calculation would be exact.
  auto result =
      m_cfg.propagator->propagateToSurface(trkParams, *planeSurface, pOptions);
  if (result.ok()) {
    return *result;
  } else {
    ACTS_ERROR("Error during propagation in estimate3DImpactParameters.");
    ACTS_DEBUG(
        "The plane surface to which we tried to propagate has its origin at\n"
        << vtxPos);
    return result.error();
  }
}

Result<double> ImpactPointEstimator::getVertexCompatibility(
    const GeometryContext& gctx, const BoundTrackParameters* trkParams,
    Eigen::Map<const ActsDynamicVector> vertexPos) const {
  if (vertexPos.size() == 3) {
    return getVertexCompatibilityImpl(gctx, trkParams,
                                      vertexPos.template head<3>());
  } else if (vertexPos.size() == 4) {
    return getVertexCompatibilityImpl(gctx, trkParams,
                                      vertexPos.template head<4>());
  } else {
    return VertexingError::InvalidInput;
  }
}

Result<std::pair<Acts::Vector4, Acts::Vector3>>
ImpactPointEstimator::getDistanceAndMomentum(
    const GeometryContext& gctx, const BoundTrackParameters& trkParams,
    Eigen::Map<const ActsDynamicVector> vtxPos, State& state) const {
  if (vtxPos.size() == 3) {
    return getDistanceAndMomentumImpl(
        gctx, trkParams, vtxPos.template head<3>(), m_cfg, state, *m_logger);
  } else if (vtxPos.size() == 4) {
    return getDistanceAndMomentumImpl(
        gctx, trkParams, vtxPos.template head<4>(), m_cfg, state, *m_logger);
  } else {
    return VertexingError::InvalidInput;
  }
}

Result<ImpactParametersAndSigma> ImpactPointEstimator::getImpactParameters(
    const BoundTrackParameters& track, const Vertex& vtx,
    const GeometryContext& gctx, const MagneticFieldContext& mctx,
    bool calculateTimeIP) const {
  const std::shared_ptr<PerigeeSurface> perigeeSurface =
      Surface::makeShared<PerigeeSurface>(vtx.position());

  // Create propagator options
  PropagatorPlainOptions pOptions(gctx, mctx);
  auto intersection =
      perigeeSurface
          ->intersect(gctx, track.position(gctx), track.direction(),
                      BoundaryTolerance::Infinite())
          .closest();
  pOptions.direction =
      Direction::fromScalarZeroAsPositive(intersection.pathLength());

  // Do the propagation to linPoint
  auto result =
      m_cfg.propagator->propagateToSurface(track, *perigeeSurface, pOptions);

  if (!result.ok()) {
    ACTS_ERROR("Error during propagation in getImpactParameters.");
    ACTS_DEBUG(
        "The Perigee surface to which we tried to propagate has its origin "
        "at\n"
        << vtx.position());
    return result.error();
  }

  const auto& params = *result;

  // Check if the covariance matrix of the Perigee parameters exists
  if (!params.covariance().has_value()) {
    return VertexingError::NoCovariance;
  }

  // Extract Perigee parameters and corresponding covariance matrix
  auto impactParams = params.impactParameters();
  auto impactParamCovariance = params.impactParameterCovariance().value();

  // Vertex variances
  // TODO: By looking at sigmaD0 and sigmaZ0 we neglect the offdiagonal terms
  // (i.e., we approximate the vertex as a sphere rather than an ellipsoid).
  // Using the full covariance matrix might furnish better results.
  double vtxVarX = vtx.covariance()(eX, eX);
  double vtxVarY = vtx.covariance()(eY, eY);
  double vtxVarZ = vtx.covariance()(eZ, eZ);

  ImpactParametersAndSigma ipAndSigma;

  ipAndSigma.d0 = impactParams[0];
  // Variance of the vertex extent in the x-y-plane
  double vtxVar2DExtent = std::max(vtxVarX, vtxVarY);
  // TODO: vtxVar2DExtent, vtxVarZ, and vtxVarT should always be >= 0. We need
  // to throw an error here once
  // https://github.com/acts-project/acts/issues/2231 is resolved.
  if (vtxVar2DExtent > 0) {
    ipAndSigma.sigmaD0 =
        std::sqrt(vtxVar2DExtent + impactParamCovariance(0, 0));
  } else {
    ipAndSigma.sigmaD0 = std::sqrt(impactParamCovariance(0, 0));
  }

  ipAndSigma.z0 = impactParams[1];
  if (vtxVarZ > 0) {
    ipAndSigma.sigmaZ0 = std::sqrt(vtxVarZ + impactParamCovariance(1, 1));
  } else {
    ipAndSigma.sigmaZ0 = std::sqrt(impactParamCovariance(1, 1));
  }

  if (calculateTimeIP) {
    ipAndSigma.deltaT = std::abs(vtx.time() - impactParams[2]);
    double vtxVarT = vtx.fullCovariance()(eTime, eTime);
    if (vtxVarT > 0) {
      ipAndSigma.sigmaDeltaT = std::sqrt(vtxVarT + impactParamCovariance(2, 2));
    } else {
      ipAndSigma.sigmaDeltaT = std::sqrt(impactParamCovariance(2, 2));
    }
  }

  return ipAndSigma;
}

Result<std::pair<double, double>> ImpactPointEstimator::getLifetimeSignOfTrack(
    const BoundTrackParameters& track, const Vertex& vtx,
    const Vector3& direction, const GeometryContext& gctx,
    const MagneticFieldContext& mctx) const {
  const std::shared_ptr<PerigeeSurface> perigeeSurface =
      Surface::makeShared<PerigeeSurface>(vtx.position());

  // Create propagator options
  PropagatorPlainOptions pOptions(gctx, mctx);
  pOptions.direction = Direction::Backward();

  // Do the propagation to the perigeee
  auto result =
      m_cfg.propagator->propagateToSurface(track, *perigeeSurface, pOptions);

  if (!result.ok()) {
    return result.error();
  }

  const auto& params = (*result).parameters();
  const double d0 = params[BoundIndices::eBoundLoc0];
  const double z0 = params[BoundIndices::eBoundLoc1];
  const double phi = params[BoundIndices::eBoundPhi];
  const double theta = params[BoundIndices::eBoundTheta];

  double vs = std::sin(std::atan2(direction[1], direction[0]) - phi) * d0;
  double eta = AngleHelpers::etaFromTheta(theta);
  double dir_eta = VectorHelpers::eta(direction);

  double zs = (dir_eta - eta) * z0;

  std::pair<double, double> vszs;

  vszs.first = vs >= 0. ? 1. : -1.;
  vszs.second = zs >= 0. ? 1. : -1.;

  return vszs;
}

Result<double> ImpactPointEstimator::get3DLifetimeSignOfTrack(
    const BoundTrackParameters& track, const Vertex& vtx,
    const Vector3& direction, const GeometryContext& gctx,
    const MagneticFieldContext& mctx) const {
  const std::shared_ptr<PerigeeSurface> perigeeSurface =
      Surface::makeShared<PerigeeSurface>(vtx.position());

  // Create propagator options
  PropagatorPlainOptions pOptions(gctx, mctx);
  pOptions.direction = Direction::Backward();

  // Do the propagation to the perigeee
  auto result =
      m_cfg.propagator->propagateToSurface(track, *perigeeSurface, pOptions);

  if (!result.ok()) {
    return result.error();
  }

  const auto& params = *result;
  const Vector3 trkpos = params.position(gctx);
  const Vector3 trkmom = params.momentum();

  double sign =
      (direction.cross(trkmom)).dot(trkmom.cross(vtx.position() - trkpos));

  return sign >= 0. ? 1. : -1.;
}

}  // namespace Acts
