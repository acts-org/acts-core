// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Vertexing/LinearizedTrack.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "Acts/Vertexing/Vertex.hpp"

namespace Acts::KalmanVertexUpdater::detail {

/// Cache object, the comments indicate the names of the variables in Ref. (1)
/// @tparam nDimVertex number of dimensions of the vertex. Can be 3 (if we only
/// fit its spatial coordinates) or 4 (if we also fit time).
template <unsigned int nDimVertex>
struct Cache {
  using VertexVector = ActsVector<nDimVertex>;
  using VertexMatrix = ActsSquareMatrix<nDimVertex>;
  // \tilde{x_k}
  VertexVector newVertexPos = VertexVector::Zero();
  // C_k
  VertexMatrix newVertexCov = VertexMatrix::Zero();
  // C_k^-1
  VertexMatrix newVertexWeight = VertexMatrix::Zero();
  // C_{k-1}^-1
  VertexMatrix oldVertexWeight = VertexMatrix::Zero();
  // W_k
  SquareMatrix3 wMat = SquareMatrix3::Zero();
};

/// @brief Calculates updated vertex position and covariance as well as the
/// updated track momentum when adding/removing linTrack. Saves the result in
/// cache.
///
/// @tparam nDimVertex number of dimensions of the vertex. Can be 3 (if we only
/// fit its spatial coordinates) or 4 (if we also fit time).
///
/// @param vtx Vertex
/// @param linTrack Linearized track to be added or removed
/// @param trackWeight Track weight
/// @param sign +1 (add track) or -1 (remove track)
/// @note Tracks are removed during the smoothing procedure to compute
/// the chi2 of the track wrt the updated vertex position
/// @param[out] cache A cache to store the results of this function
template <unsigned int nDimVertex>
void calculateUpdate(const Vertex& vtx, const Acts::LinearizedTrack& linTrack,
                     const double trackWeight, const int sign,
                     Cache<nDimVertex>& cache) {
  constexpr unsigned int nBoundParams = nDimVertex + 2;
  using ParameterVector = ActsVector<nBoundParams>;
  using ParameterMatrix = ActsSquareMatrix<nBoundParams>;
  // Retrieve variables from the track linearization. The comments indicate the
  // corresponding symbol used in Ref. (1).
  // A_k
  const ActsMatrix<nBoundParams, nDimVertex> posJac =
      linTrack.positionJacobian.block<nBoundParams, nDimVertex>(0, 0);
  // B_k
  const ActsMatrix<nBoundParams, 3> momJac =
      linTrack.momentumJacobian.block<nBoundParams, 3>(0, 0);
  // p_k
  const ParameterVector trkParams =
      linTrack.parametersAtPCA.head<nBoundParams>();
  // c_k
  const ParameterVector constTerm = linTrack.constantTerm.head<nBoundParams>();
  // TODO we could use `linTrack.weightAtPCA` but only if we would always fit
  // time.
  // G_k
  // Note that, when removing a track, G_k -> - G_k, see Ref. (1).
  // Further note that, as we use the weighted formalism, the track weight
  // matrix (i.e., the inverse track covariance matrix) should be multiplied
  // with the track weight from the AMVF formalism. Here, we choose to
  // consider these two multiplicative factors directly in the updates of
  // newVertexWeight and newVertexPos.
  const ParameterMatrix trkParamWeight =
      linTrack.covarianceAtPCA.block<nBoundParams, nBoundParams>(0, 0)
          .inverse();

  // Retrieve current position of the vertex and its current weight matrix
  const ActsVector<nDimVertex> oldVtxPos =
      vtx.fullPosition().template head<nDimVertex>();
  // C_{k-1}^-1
  cache.oldVertexWeight =
      (vtx.fullCovariance().template block<nDimVertex, nDimVertex>(0, 0))
          .inverse();

  // W_k
  cache.wMat = (momJac.transpose() * (trkParamWeight * momJac)).inverse();

  // G_k^B = G_k - G_k*B_k*W_k*B_k^(T)*G_k
  ParameterMatrix gBMat = trkParamWeight - trkParamWeight * momJac *
                                               cache.wMat * momJac.transpose() *
                                               trkParamWeight;

  // C_k^-1
  cache.newVertexWeight = cache.oldVertexWeight + sign * trackWeight *
                                                      posJac.transpose() *
                                                      gBMat * posJac;
  // C_k
  cache.newVertexCov = cache.newVertexWeight.inverse();

  // \tilde{x_k}
  cache.newVertexPos =
      cache.newVertexCov * (cache.oldVertexWeight * oldVtxPos +
                            sign * trackWeight * posJac.transpose() * gBMat *
                                (trkParams - constTerm));
}

template <unsigned int nDimVertex>
double vertexPositionChi2Update(const Vector4& oldVtxPos,
                                const Cache<nDimVertex>& cache) {
  ActsVector<nDimVertex> posDiff =
      cache.newVertexPos - oldVtxPos.template head<nDimVertex>();

  // Calculate and return corresponding chi2
  return posDiff.transpose() * (cache.oldVertexWeight * posDiff);
}

template <unsigned int nDimVertex>
double trackParametersChi2(const LinearizedTrack& linTrack,
                           const Cache<nDimVertex>& cache) {
  constexpr unsigned int nBoundParams = nDimVertex + 2;
  using ParameterVector = ActsVector<nBoundParams>;
  using ParameterMatrix = ActsSquareMatrix<nBoundParams>;
  // A_k
  const ActsMatrix<nBoundParams, nDimVertex> posJac =
      linTrack.positionJacobian.block<nBoundParams, nDimVertex>(0, 0);
  // B_k
  const ActsMatrix<nBoundParams, 3> momJac =
      linTrack.momentumJacobian.block<nBoundParams, 3>(0, 0);
  // p_k
  const ParameterVector trkParams =
      linTrack.parametersAtPCA.head<nBoundParams>();
  // c_k
  const ParameterVector constTerm = linTrack.constantTerm.head<nBoundParams>();
  // TODO we could use `linTrack.weightAtPCA` but only if we would always fit
  // time.
  // G_k
  const ParameterMatrix trkParamWeight =
      linTrack.covarianceAtPCA.block<nBoundParams, nBoundParams>(0, 0)
          .inverse();

  // A_k * \tilde{x_k}
  const ParameterVector posJacVtxPos = posJac * cache.newVertexPos;

  // \tilde{q_k}
  Vector3 newTrkMom = cache.wMat * momJac.transpose() * trkParamWeight *
                      (trkParams - constTerm - posJacVtxPos);

  // \tilde{p_k}
  ParameterVector linearizedTrackParameters =
      constTerm + posJacVtxPos + momJac * newTrkMom;

  // r_k
  ParameterVector paramDiff = trkParams - linearizedTrackParameters;

  // Return chi2
  return paramDiff.transpose() * (trkParamWeight * paramDiff);
}

/// @brief Calculates a covariance matrix for the refitted track parameters
///
/// @tparam nDimVertex number of dimensions of the vertex. Can be 3 (if we only
/// fit its spatial coordinates) or 4 (if we also fit time).
///
/// @param wMat W_k matrix from Ref. (1)
/// @param crossCovVP Cross-covariance matrix between vertex position and track
/// momentum
/// @param vtxCov Vertex covariance matrix
/// @param newTrkParams Refitted track parameters
template <unsigned int nDimVertex>
Acts::BoundMatrix calculateTrackCovariance(
    const SquareMatrix3& wMat, const ActsMatrix<nDimVertex, 3>& crossCovVP,
    const ActsSquareMatrix<nDimVertex>& vtxCov,
    const BoundVector& newTrkParams) {
  // D_k^n
  ActsSquareMatrix<3> momCov =
      wMat + crossCovVP.transpose() * vtxCov.inverse() * crossCovVP;

  // Full x, y, z, phi, theta, q/p, and, optionally, t covariance matrix. Note
  // that we call this set of parameters "free" in the following even though
  // that is not quite correct (free parameters correspond to
  // x, y, z, t, px, py, pz)
  constexpr unsigned int nFreeParams = nDimVertex + 3;
  ActsSquareMatrix<nFreeParams> freeTrkCov(
      ActsSquareMatrix<nFreeParams>::Zero());

  freeTrkCov.template block<3, 3>(0, 0) = vtxCov.template block<3, 3>(0, 0);
  freeTrkCov.template block<3, 3>(0, 3) = crossCovVP.template block<3, 3>(0, 0);
  freeTrkCov.template block<3, 3>(3, 0) =
      crossCovVP.template block<3, 3>(0, 0).transpose();
  freeTrkCov.template block<3, 3>(3, 3) = momCov;

  // Fill time (cross-)covariances
  if constexpr (nFreeParams == 7) {
    freeTrkCov.template block<3, 1>(0, 6) = vtxCov.template block<3, 1>(0, 3);
    freeTrkCov.template block<3, 1>(3, 6) =
        crossCovVP.template block<1, 3>(3, 0).transpose();
    freeTrkCov.template block<1, 3>(6, 0) = vtxCov.template block<1, 3>(3, 0);
    freeTrkCov.template block<1, 3>(6, 3) =
        crossCovVP.template block<1, 3>(3, 0);
    freeTrkCov(6, 6) = vtxCov(3, 3);
  }

  // Jacobian relating "free" and bound track parameters
  constexpr unsigned int nBoundParams = nDimVertex + 2;
  ActsMatrix<nBoundParams, nFreeParams> freeToBoundJac(
      ActsMatrix<nBoundParams, nFreeParams>::Zero());

  // TODO: Jacobian is not quite correct
  // First row
  freeToBoundJac(0, 0) = -std::sin(newTrkParams[2]);
  freeToBoundJac(0, 1) = std::cos(newTrkParams[2]);

  double tanTheta = std::tan(newTrkParams[3]);

  // Second row
  freeToBoundJac(1, 0) = -freeToBoundJac(0, 1) / tanTheta;
  freeToBoundJac(1, 1) = freeToBoundJac(0, 0) / tanTheta;

  // Dimension of the part of the Jacobian that is an identity matrix
  constexpr unsigned int nDimIdentity = nFreeParams - 2;
  freeToBoundJac.template block<nDimIdentity, nDimIdentity>(1, 2) =
      ActsMatrix<nDimIdentity, nDimIdentity>::Identity();

  // Full perigee track covariance
  BoundMatrix boundTrackCov(BoundMatrix::Identity());
  boundTrackCov.block<nBoundParams, nBoundParams>(0, 0) =
      (freeToBoundJac * (freeTrkCov * freeToBoundJac.transpose()));

  return boundTrackCov;
}

template <unsigned int nDimVertex>
void updateVertexWithTrackImpl(Vertex& vtx, TrackAtVertex& trk, int sign) {
  if constexpr (nDimVertex != 3 && nDimVertex != 4) {
    throw std::invalid_argument(
        "The vertex dimension must either be 3 (when fitting the spatial "
        "coordinates) or 4 (when fitting the spatial coordinates + time).");
  }

  double trackWeight = trk.trackWeight;

  // Set up cache where entire content is set to 0
  Cache<nDimVertex> cache;

  // Calculate update and save result in cache
  calculateUpdate(vtx, trk.linearizedState, trackWeight, sign, cache);

  // Get fit quality parameters wrt to old vertex
  auto [chi2, ndf] = vtx.fitQuality();

  // Chi2 of the track parameters
  double trkChi2 = trackParametersChi2(trk.linearizedState, cache);

  // Update of the chi2 of the vertex position
  double vtxPosChi2Update = vertexPositionChi2Update(vtx.fullPosition(), cache);

  // Calculate new chi2
  chi2 += sign * (vtxPosChi2Update + trackWeight * trkChi2);

  // Calculate ndf
  ndf += sign * trackWeight * 2.;

  // Updating the vertex
  if constexpr (nDimVertex == 3) {
    vtx.fullPosition().head<3>() = cache.newVertexPos.template head<3>();
    vtx.fullCovariance().template topLeftCorner<3, 3>() =
        cache.newVertexCov.template topLeftCorner<3, 3>();
  } else if constexpr (nDimVertex == 4) {
    vtx.fullPosition() = cache.newVertexPos;
    vtx.fullCovariance() = cache.newVertexCov;
  }
  vtx.setFitQuality(chi2, ndf);

  if (sign == 1) {
    // Update track
    trk.chi2Track = trkChi2;
    trk.ndf = 2 * trackWeight;
  }
  // Remove trk from current vertex by setting its weight to 0
  else if (sign == -1) {
    trk.trackWeight = 0.;
  } else {
    throw std::invalid_argument(
        "Sign for adding/removing track must be +1 (add) or -1 (remove).");
  }
}

template <unsigned int nDimVertex>
void updateTrackWithVertexImpl(TrackAtVertex& track, const Vertex& vtx) {
  if constexpr (nDimVertex != 3 && nDimVertex != 4) {
    throw std::invalid_argument(
        "The vertex dimension must either be 3 (when fitting the spatial "
        "coordinates) or 4 (when fitting the spatial coordinates + time).");
  }

  using VertexVector = ActsVector<nDimVertex>;
  using VertexMatrix = ActsSquareMatrix<nDimVertex>;
  constexpr unsigned int nBoundParams = nDimVertex + 2;
  using ParameterVector = ActsVector<nBoundParams>;
  using ParameterMatrix = ActsSquareMatrix<nBoundParams>;
  // Check if linearized state exists
  if (!track.isLinearized) {
    throw std::invalid_argument("TrackAtVertex object must be linearized.");
  }

  // Extract vertex position and covariance
  // \tilde{x_n}
  const VertexVector vtxPos = vtx.fullPosition().template head<nDimVertex>();
  // C_n
  const VertexMatrix vtxCov =
      vtx.fullCovariance().template block<nDimVertex, nDimVertex>(0, 0);

  // Get the linearized track
  const LinearizedTrack& linTrack = track.linearizedState;
  // Retrieve variables from the track linearization. The comments indicate the
  // corresponding symbol used in Ref. (1).
  // A_k
  const ActsMatrix<nBoundParams, nDimVertex> posJac =
      linTrack.positionJacobian.block<nBoundParams, nDimVertex>(0, 0);
  // B_k
  const ActsMatrix<nBoundParams, 3> momJac =
      linTrack.momentumJacobian.block<nBoundParams, 3>(0, 0);
  // p_k
  const ParameterVector trkParams =
      linTrack.parametersAtPCA.head<nBoundParams>();
  // c_k
  const ParameterVector constTerm = linTrack.constantTerm.head<nBoundParams>();
  // TODO we could use `linTrack.weightAtPCA` but only if we would always fit
  // time.
  // G_k
  const ParameterMatrix trkParamWeight =
      linTrack.covarianceAtPCA.block<nBoundParams, nBoundParams>(0, 0)
          .inverse();

  // Cache object filled with zeros
  Cache<nDimVertex> cache;

  // Calculate the update of the vertex position when the track is removed. This
  // might be unintuitive, but it is needed to compute a symmetric chi2.
  calculateUpdate(vtx, linTrack, track.trackWeight, -1, cache);

  // Refit track momentum with the final vertex position
  Vector3 newTrkMomentum = cache.wMat * momJac.transpose() * trkParamWeight *
                           (trkParams - constTerm - posJac * vtxPos);

  // Track parameters, impact parameters are set to 0 and momentum corresponds
  // to newTrkMomentum. TODO: Make transition fitterParams -> fittedMomentum.
  BoundVector newTrkParams(BoundVector::Zero());

  // Get phi and theta and correct for possible periodicity changes
  const auto correctedPhiTheta =
      Acts::detail::normalizePhiTheta(newTrkMomentum(0), newTrkMomentum(1));
  newTrkParams(BoundIndices::eBoundPhi) = correctedPhiTheta.first;     // phi
  newTrkParams(BoundIndices::eBoundTheta) = correctedPhiTheta.second;  // theta
  newTrkParams(BoundIndices::eBoundQOverP) = newTrkMomentum(2);        // qOverP

  // E_k^n
  const ActsMatrix<nDimVertex, 3> crossCovVP =
      -vtxCov * posJac.transpose() * trkParamWeight * momJac * cache.wMat;

  // Difference in positions. cache.newVertexPos corresponds to \tilde{x_k^{n*}} in Ref. (1).
  VertexVector posDiff =
      vtxPos - cache.newVertexPos.template head<nDimVertex>();

  // r_k^n
  ParameterVector paramDiff =
      trkParams - (constTerm + posJac * vtxPos + momJac * newTrkMomentum);

  // New chi2 to be set later
  double chi2 =
      posDiff.dot(
          cache.newVertexWeight.template block<nDimVertex, nDimVertex>(0, 0) *
          posDiff) +
      paramDiff.dot(trkParamWeight * paramDiff);

  Acts::BoundMatrix newTrackCov = calculateTrackCovariance<nDimVertex>(
      cache.wMat, crossCovVP, vtxCov, newTrkParams);

  // Create new refitted parameters
  std::shared_ptr<PerigeeSurface> perigeeSurface =
      Surface::makeShared<PerigeeSurface>(vtxPos.template head<3>());

  BoundTrackParameters refittedPerigee =
      BoundTrackParameters(perigeeSurface, newTrkParams, std::move(newTrackCov),
                           track.fittedParams.particleHypothesis());

  // Set new properties
  track.fittedParams = refittedPerigee;
  track.chi2Track = chi2;
  track.ndf = 2 * track.trackWeight;

  return;
}

}  // namespace Acts::KalmanVertexUpdater::detail
