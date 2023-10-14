// This file is part of the Acts project.
//
// Copyright (C) 2019-2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Vertexing/VertexingError.hpp"

#include <algorithm>

template <typename input_track_t>
void Acts::KalmanVertexUpdater::updateVertexWithTrack(
    Vertex<input_track_t>& vtx, TrackAtVertex<input_track_t>& trk) {
  detail::update<input_track_t>(vtx, trk, 1);
}

template <typename input_track_t>
void Acts::KalmanVertexUpdater::detail::update(
    Vertex<input_track_t>& vtx, TrackAtVertex<input_track_t>& trk, int sign) {
  double trackWeight = trk.trackWeight;

  MatrixCache matrixCache;

  updatePosition(vtx, trk.linearizedState, trackWeight, sign, matrixCache);

  // Get fit quality parameters wrt to old vertex
  std::pair fitQuality = vtx.fitQuality();
  double chi2 = fitQuality.first;
  double ndf = fitQuality.second;

  // Chi2 wrt to track parameters
  double trkChi2 = detail::trackParametersChi2<input_track_t>(
      trk.linearizedState, matrixCache);

  // Calculate new chi2
  chi2 += sign * (detail::vertexPositionChi2<input_track_t>(vtx, matrixCache) +
                  trackWeight * trkChi2);

  // Calculate ndf
  ndf += sign * trackWeight * 2.;

  // Updating the vertex
  vtx.setFullPosition(matrixCache.newVertexPos);
  vtx.setFullCovariance(matrixCache.newVertexCov);
  vtx.setFitQuality(chi2, ndf);

  // Updates track at vertex if already there
  // by removing it first and adding new one.
  // Otherwise just adds track to existing list of tracks at vertex
  if (sign > 0) {
    // Update track
    trk.chi2Track = trkChi2;
    trk.ndf = 2 * trackWeight;
  }
  // Remove trk from current vertex
  if (sign < 0) {
    trk.trackWeight = 0;
  }
}

template <typename input_track_t>
void Acts::KalmanVertexUpdater::updatePosition(
    const Acts::Vertex<input_track_t>& vtx,
    const Acts::LinearizedTrack& linTrack, double trackWeight, int sign,
    MatrixCache& matrixCache) {
  // Retrieve linTrack information
  const ActsMatrix<eBoundSize, 4> posJac = linTrack.positionJacobian;
  const ActsMatrix<eBoundSize, 3> momJac =
      linTrack.momentumJacobian;  // B_k in comments below
  const BoundVector trkParams = linTrack.parametersAtPCA;
  const BoundVector constTerm = linTrack.constantTerm;
  const BoundSquareMatrix trkParamWeight = linTrack.weightAtPCA;

  // Vertex to be updated
  const Vector4& oldVtxPos = vtx.fullPosition();
  matrixCache.oldVertexWeight = (vtx.fullCovariance()).inverse();

  // W_k matrix
  matrixCache.momWeightInv =
      (momJac.transpose() * (trkParamWeight * momJac)).inverse();

  // G_k^B = G_k - G_k*B_k*W_k*B_k^(T)*G_k^T
  BoundSquareMatrix gMat =
      trkParamWeight -
      trkParamWeight *
          (momJac * (matrixCache.momWeightInv * momJac.transpose())) *
          trkParamWeight.transpose();

  // New vertex cov matrix
  matrixCache.newVertexWeight =
      matrixCache.oldVertexWeight +
      trackWeight * sign * posJac.transpose() * (gMat * posJac);
  matrixCache.newVertexCov = matrixCache.newVertexWeight.inverse();

  // New vertex position
  matrixCache.newVertexPos =
      matrixCache.newVertexCov * (matrixCache.oldVertexWeight * oldVtxPos +
                                  trackWeight * sign * posJac.transpose() *
                                      gMat * (trkParams - constTerm));
}

template <typename input_track_t>
double Acts::KalmanVertexUpdater::detail::vertexPositionChi2(
    const Vertex<input_track_t>& oldVtx, const MatrixCache& matrixCache) {
  Vector4 posDiff = matrixCache.newVertexPos - oldVtx.fullPosition();

  // Calculate and return corresponding chi2
  return posDiff.transpose() * (matrixCache.oldVertexWeight * posDiff);
}

template <typename input_track_t>
double Acts::KalmanVertexUpdater::detail::trackParametersChi2(
    const LinearizedTrack& linTrack, const MatrixCache& matrixCache) {
  // Track properties
  const ActsMatrix<eBoundSize, 4> posJac = linTrack.positionJacobian;
  const ActsMatrix<eBoundSize, 3> momJac = linTrack.momentumJacobian;
  const BoundVector trkParams = linTrack.parametersAtPCA;
  const BoundVector constTerm = linTrack.constantTerm;
  const BoundSquareMatrix trkParamWeight = linTrack.weightAtPCA;

  const BoundVector jacVtx = posJac * matrixCache.newVertexPos;

  // Refitted track momentum
  Vector3 newTrackMomentum = matrixCache.momWeightInv * momJac.transpose() *
                             trkParamWeight * (trkParams - constTerm - jacVtx);

  // Refitted track parameters
  BoundVector newTrkParams = constTerm + jacVtx + momJac * newTrackMomentum;

  // Parameter difference
  BoundVector paramDiff = trkParams - newTrkParams;

  // Return chi2
  return paramDiff.transpose() * (trkParamWeight * paramDiff);
}
