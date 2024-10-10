// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/AdaptiveGridDensityVertexFinder.hpp"

Acts::Result<std::vector<Acts::Vertex>>
Acts::AdaptiveGridDensityVertexFinder::find(
    const std::vector<InputTrack>& trackVector,
    const VertexingOptions& vertexingOptions,
    IVertexFinder::State& anyState) const {
  auto& state = anyState.as<State>();
  // Remove density contributions from tracks removed from track collection
  if (m_cfg.cacheGridStateForTrackRemoval && state.isInitialized &&
      !state.tracksToRemove.empty()) {
    for (auto trk : state.tracksToRemove) {
      auto it = state.trackDensities.find(trk);
      if (it == state.trackDensities.end()) {
        // Track was never added to grid, so cannot remove it
        continue;
      }
      m_cfg.gridDensity.subtractTrack(it->second, state.mainDensityMap);
    }
  } else {
    state.mainDensityMap = DensityMap();
    // Fill with track densities
    for (auto trk : trackVector) {
      const BoundTrackParameters& trkParams = m_cfg.extractParameters(trk);
      // Take only tracks that fulfill selection criteria
      if (!doesPassTrackSelection(trkParams)) {
        continue;
      }
      auto trackDensityMap =
          m_cfg.gridDensity.addTrack(trkParams, state.mainDensityMap);
      // Cache track density contribution to main grid if enabled
      if (m_cfg.cacheGridStateForTrackRemoval) {
        state.trackDensities[trk] = std::move(trackDensityMap);
      }
    }
    state.isInitialized = true;
  }

  if (state.mainDensityMap.empty()) {
    // No tracks passed selection
    // Return empty seed
    // (Note: Upstream finder should check for this break condition)
    return std::vector<Vertex>{};
  }

  double z = 0;
  double t = 0;
  double zWidth = 0;

  if (!m_cfg.estimateSeedWidth) {
    // Get z value of highest density bin
    auto maxZTRes = m_cfg.gridDensity.getMaxZTPosition(state.mainDensityMap);

    if (!maxZTRes.ok()) {
      return maxZTRes.error();
    }
    z = (*maxZTRes).first;
    t = (*maxZTRes).second;
  } else {
    // Get z value of highest density bin and width
    auto maxZTResAndWidth =
        m_cfg.gridDensity.getMaxZTPositionAndWidth(state.mainDensityMap);

    if (!maxZTResAndWidth.ok()) {
      return maxZTResAndWidth.error();
    }
    z = (*maxZTResAndWidth).first.first;
    t = (*maxZTResAndWidth).first.second;
    zWidth = (*maxZTResAndWidth).second;
  }

  // Construct output vertex, t will be 0 if temporalTrkGridSize == 1
  Vector4 seedPos =
      vertexingOptions.constraint.fullPosition() + Vector4(0., 0., z, t);

  Vertex returnVertex = Vertex(seedPos);

  SquareMatrix4 seedCov = vertexingOptions.constraint.fullCovariance();

  if (zWidth != 0.) {
    // Use z-constraint from seed width
    seedCov(2, 2) = zWidth * zWidth;
  }

  returnVertex.setFullCovariance(seedCov);

  return std::vector<Vertex>{returnVertex};
}

bool Acts::AdaptiveGridDensityVertexFinder::doesPassTrackSelection(
    const BoundTrackParameters& trk) const {
  // Get required track parameters
  const double d0 = trk.parameters()[BoundIndices::eBoundLoc0];
  const double z0 = trk.parameters()[BoundIndices::eBoundLoc1];
  // Get track covariance
  if (!trk.covariance().has_value()) {
    return false;
  }
  const auto perigeeCov = *(trk.covariance());
  const double covDD =
      perigeeCov(BoundIndices::eBoundLoc0, BoundIndices::eBoundLoc0);
  const double covZZ =
      perigeeCov(BoundIndices::eBoundLoc1, BoundIndices::eBoundLoc1);
  const double covDZ =
      perigeeCov(BoundIndices::eBoundLoc0, BoundIndices::eBoundLoc1);
  const double covDeterminant = covDD * covZZ - covDZ * covDZ;

  // Do track selection based on track cov matrix and d0SignificanceCut
  if ((covDD <= 0) || (d0 * d0 / covDD > m_cfg.d0SignificanceCut) ||
      (covZZ <= 0) || (covDeterminant <= 0)) {
    return false;
  }

  // Calculate track density quantities to check if track can easily
  // be considered as 2-dim Gaussian distribution without causing problems
  double constantTerm =
      -(d0 * d0 * covZZ + z0 * z0 * covDD + 2. * d0 * z0 * covDZ) /
      (2. * covDeterminant);
  const double linearTerm = (d0 * covDZ + z0 * covDD) / covDeterminant;
  const double quadraticTerm = -covDD / (2. * covDeterminant);
  double discriminant =
      linearTerm * linearTerm -
      4. * quadraticTerm * (constantTerm + 2. * m_cfg.z0SignificanceCut);
  if (discriminant < 0) {
    return false;
  }

  return true;
}
