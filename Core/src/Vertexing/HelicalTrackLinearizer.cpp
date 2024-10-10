// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/HelicalTrackLinearizer.hpp"

#include "Acts/Propagator/PropagatorOptions.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Utilities/MathHelpers.hpp"
#include "Acts/Vertexing/LinearizerTrackParameters.hpp"

Acts::Result<Acts::LinearizedTrack>
Acts::HelicalTrackLinearizer::linearizeTrack(
    const BoundTrackParameters& params, double linPointTime,
    const Surface& perigeeSurface, const Acts::GeometryContext& gctx,
    const Acts::MagneticFieldContext& mctx,
    MagneticFieldProvider::Cache& fieldCache) const {
  // Create propagator options
  PropagatorPlainOptions pOptions(gctx, mctx);

  // Length scale at which we consider to be sufficiently close to the Perigee
  // surface to skip the propagation.
  pOptions.surfaceTolerance = m_cfg.targetTolerance;

  // Get intersection of the track with the Perigee if the particle would
  // move on a straight line.
  // This allows us to determine whether we need to propagate the track
  // forward or backward to arrive at the PCA.
  auto intersection =
      perigeeSurface
          .intersect(gctx, params.position(gctx), params.direction(),
                     BoundaryTolerance::Infinite())
          .closest();

  // Setting the propagation direction using the intersection length from
  // above
  // We handle zero path length as forward propagation, but we could actually
  // skip the whole propagation in this case
  pOptions.direction =
      Direction::fromScalarZeroAsPositive(intersection.pathLength());

  // Propagate to the PCA of the reference point
  const auto res =
      m_cfg.propagator->propagateToSurface(params, perigeeSurface, pOptions);
  if (!res.ok()) {
    return res.error();
  }
  const auto& endParams = *res;

  // Extracting the track parameters at said PCA - this corresponds to the
  // Perigee representation of the track wrt the reference point
  BoundVector paramsAtPCA = endParams.parameters();

  // Extracting the 4D position of the PCA in global coordinates
  Vector4 pca = Vector4::Zero();
  {
    auto pos = endParams.position(gctx);
    pca[ePos0] = pos[ePos0];
    pca[ePos1] = pos[ePos1];
    pca[ePos2] = pos[ePos2];
    pca[eTime] = endParams.time();
  }
  BoundSquareMatrix parCovarianceAtPCA = endParams.covariance().value();

  // Extracting Perigee parameters and compute functions of them for later
  // usage
  ActsScalar d0 = paramsAtPCA(BoundIndices::eBoundLoc0);

  ActsScalar phi = paramsAtPCA(BoundIndices::eBoundPhi);
  ActsScalar sinPhi = std::sin(phi);
  ActsScalar cosPhi = std::cos(phi);

  ActsScalar theta = paramsAtPCA(BoundIndices::eBoundTheta);
  ActsScalar sinTheta = std::sin(theta);
  ActsScalar tanTheta = std::tan(theta);

  // q over p
  ActsScalar qOvP = paramsAtPCA(BoundIndices::eBoundQOverP);
  // Rest mass
  ActsScalar m0 = params.particleHypothesis().mass();
  // Momentum
  ActsScalar p = params.particleHypothesis().extractMomentum(qOvP);

  // Speed in units of c
  ActsScalar beta = p / fastHypot(p, m0);
  // Transverse speed (i.e., speed in the x-y plane)
  ActsScalar betaT = beta * sinTheta;

  // Momentum direction at the PCA
  Vector3 momentumAtPCA(phi, theta, qOvP);

  // Particle charge
  ActsScalar absoluteCharge = params.particleHypothesis().absoluteCharge();

  // get the z-component of the B-field at the PCA
  auto field = m_cfg.bField->getField(VectorHelpers::position(pca), fieldCache);
  if (!field.ok()) {
    return field.error();
  }
  ActsScalar Bz = (*field)[eZ];

  // Complete Jacobian (consists of positionJacobian and momentumJacobian)
  ActsMatrix<eBoundSize, eLinSize> completeJacobian =
      ActsMatrix<eBoundSize, eLinSize>::Zero(eBoundSize, eLinSize);

  // The particle moves on a straight trajectory if its charge is 0 or if there
  // is no B field. Conversely, if it has a charge and the B field is constant,
  // it moves on a helical trajectory.
  if (absoluteCharge == 0. || Bz == 0.) {
    // Derivatives can be found in Eqs. 5.39 and 5.40 of Ref. (1).
    // Since we propagated to the PCA (point P in Ref. (1)), we evaluate the
    // Jacobians there. One can show that, in this case, RTilde = 0 and QTilde =
    // -d0.

    // Derivatives of d0
    completeJacobian(eBoundLoc0, eLinPos0) = -sinPhi;
    completeJacobian(eBoundLoc0, eLinPos1) = cosPhi;

    // Derivatives of z0
    completeJacobian(eBoundLoc1, eLinPos0) = -cosPhi / tanTheta;
    completeJacobian(eBoundLoc1, eLinPos1) = -sinPhi / tanTheta;
    completeJacobian(eBoundLoc1, eLinPos2) = 1.;
    completeJacobian(eBoundLoc1, eLinPhi) = -d0 / tanTheta;

    // Derivatives of phi
    completeJacobian(eBoundPhi, eLinPhi) = 1.;

    // Derivatives of theta
    completeJacobian(eBoundTheta, eLinTheta) = 1.;

    // Derivatives of q/p
    completeJacobian(eBoundQOverP, eLinQOverP) = 1.;

    // Derivatives of time
    completeJacobian(eBoundTime, eLinPos0) = -cosPhi / betaT;
    completeJacobian(eBoundTime, eLinPos1) = -sinPhi / betaT;
    completeJacobian(eBoundTime, eLinTime) = 1.;
    completeJacobian(eBoundTime, eLinPhi) = -d0 / betaT;
  } else {
    // Helix radius
    ActsScalar rho = sinTheta * (1. / qOvP) / Bz;
    // Sign of helix radius
    ActsScalar h = (rho < 0.) ? -1 : 1;

    // Quantities from Eq. 5.34 in Ref. (1) (see .hpp)
    ActsScalar X = pca(0) - perigeeSurface.center(gctx).x() + rho * sinPhi;
    ActsScalar Y = pca(1) - perigeeSurface.center(gctx).y() - rho * cosPhi;
    ActsScalar S2 = (X * X + Y * Y);
    // S is the 2D distance from the helix center to the reference point
    // in the x-y plane
    ActsScalar S = std::sqrt(S2);

    ActsScalar XoverS2 = X / S2;
    ActsScalar YoverS2 = Y / S2;
    ActsScalar rhoCotTheta = rho / tanTheta;
    ActsScalar rhoOverBetaT = rho / betaT;
    // Absolute value of rho over S
    ActsScalar absRhoOverS = h * rho / S;

    // Derivatives can be found in Eq. 5.36 in Ref. (1)
    // Since we propagated to the PCA (point P in Ref. (1)), the points
    // P and V coincide, and thus deltaPhi = 0.
    // One can show that if deltaPhi = 0 --> R = 0 and Q = h * S.
    // As a consequence, many terms of the B matrix vanish.

    // Derivatives of d0
    completeJacobian(eBoundLoc0, eLinPos0) = -h * X / S;
    completeJacobian(eBoundLoc0, eLinPos1) = -h * Y / S;

    // Derivatives of z0
    completeJacobian(eBoundLoc1, eLinPos0) = rhoCotTheta * YoverS2;
    completeJacobian(eBoundLoc1, eLinPos1) = -rhoCotTheta * XoverS2;
    completeJacobian(eBoundLoc1, eLinPos2) = 1.;
    completeJacobian(eBoundLoc1, eLinPhi) = rhoCotTheta * (1. - absRhoOverS);

    // Derivatives of phi
    completeJacobian(eBoundPhi, eLinPos0) = -YoverS2;
    completeJacobian(eBoundPhi, eLinPos1) = XoverS2;
    completeJacobian(eBoundPhi, eLinPhi) = absRhoOverS;

    // Derivatives of theta
    completeJacobian(eBoundTheta, eLinTheta) = 1.;

    // Derivatives of q/p
    completeJacobian(eBoundQOverP, eLinQOverP) = 1.;

    // Derivatives of time
    completeJacobian(eBoundTime, eLinPos0) = rhoOverBetaT * YoverS2;
    completeJacobian(eBoundTime, eLinPos1) = -rhoOverBetaT * XoverS2;
    completeJacobian(eBoundTime, eLinTime) = 1.;
    completeJacobian(eBoundTime, eLinPhi) = rhoOverBetaT * (1. - absRhoOverS);
  }

  // Extracting positionJacobian and momentumJacobian from the complete Jacobian
  ActsMatrix<eBoundSize, eLinPosSize> positionJacobian =
      completeJacobian.block<eBoundSize, eLinPosSize>(0, 0);
  ActsMatrix<eBoundSize, eLinMomSize> momentumJacobian =
      completeJacobian.block<eBoundSize, eLinMomSize>(0, eLinPosSize);

  // const term in Taylor expansion from Eq. 5.38 in Ref. (1)
  BoundVector constTerm =
      paramsAtPCA - positionJacobian * pca - momentumJacobian * momentumAtPCA;

  // The parameter weight
  BoundSquareMatrix weightAtPCA = parCovarianceAtPCA.inverse();

  Vector4 linPoint;
  linPoint.head<3>() = perigeeSurface.center(gctx);
  linPoint[3] = linPointTime;

  return LinearizedTrack(paramsAtPCA, parCovarianceAtPCA, weightAtPCA, linPoint,
                         positionJacobian, momentumJacobian, pca, momentumAtPCA,
                         constTerm);
}
