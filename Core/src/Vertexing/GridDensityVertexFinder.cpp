// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/GridDensityVertexFinder.hpp"

namespace Acts {

auto GridDensityVertexFinder::find(const std::vector<InputTrack>& trackVector,
                                   const VertexingOptions& vertexingOptions,
                                   IVertexFinder::State& anyState) const
    -> Result<std::vector<Vertex>> {
  auto& state = anyState.as<State>();
  // Remove density contributions from tracks removed from track collection
  if (m_cfg.cacheGridStateForTrackRemoval && state.isInitialized &&
      !state.tracksToRemove.empty()) {
    // Bool to check if removable tracks, that pass selection, still exist
    bool couldRemoveTracks = false;
    for (auto trk : state.tracksToRemove) {
      if (!state.trackSelectionMap.at(trk)) {
        // Track was never added to grid, so cannot remove it
        continue;
      }
      couldRemoveTracks = true;
      auto binAndTrackGrid = state.binAndTrackGridMap.at(trk);
      m_cfg.gridDensity.removeTrackGridFromMainGrid(
          binAndTrackGrid.first, binAndTrackGrid.second, state.mainGrid);
    }
    if (!couldRemoveTracks) {
      // No tracks were removed anymore
      // Return empty seed
      // (Note: Upstream finder should check for this break condition)
      return std::vector<Vertex>{};
    }
  } else {
    state.mainGrid =
        MainGridVector::Zero(m_cfg.gridDensity.config().mainGridSize);
    // Fill with track densities
    for (auto trk : trackVector) {
      const BoundTrackParameters& trkParams = m_cfg.extractParameters(trk);
      // Take only tracks that fulfill selection criteria
      if (!doesPassTrackSelection(trkParams)) {
        if (m_cfg.cacheGridStateForTrackRemoval) {
          state.trackSelectionMap[trk] = false;
        }
        continue;
      }
      auto binAndTrackGrid =
          m_cfg.gridDensity.addTrack(trkParams, state.mainGrid);
      // Cache track density contribution to main grid if enabled
      if (m_cfg.cacheGridStateForTrackRemoval) {
        state.binAndTrackGridMap[trk] = binAndTrackGrid;
        state.trackSelectionMap[trk] = true;
      }
    }
    state.isInitialized = true;
  }

  double z = 0;
  double width = 0;
  if (!state.mainGrid.isZero()) {
    if (!m_cfg.estimateSeedWidth) {
      // Get z value of highest density bin
      auto maxZres = m_cfg.gridDensity.getMaxZPosition(state.mainGrid);

      if (!maxZres.ok()) {
        return maxZres.error();
      }
      z = *maxZres;
    } else {
      // Get z value of highest density bin and width
      auto maxZres = m_cfg.gridDensity.getMaxZPositionAndWidth(state.mainGrid);

      if (!maxZres.ok()) {
        return maxZres.error();
      }
      z = (*maxZres).first;
      width = (*maxZres).second;
    }
  }

  // Construct output vertex
  Vector3 seedPos = vertexingOptions.constraint.position() + Vector3(0., 0., z);

  Vertex returnVertex = Vertex(seedPos);

  SquareMatrix4 seedCov = vertexingOptions.constraint.fullCovariance();

  if (width != 0.) {
    // Use z-constraint from seed width
    seedCov(2, 2) = width * width;
  }

  returnVertex.setFullCovariance(seedCov);

  return std::vector<Vertex>{returnVertex};
}

auto GridDensityVertexFinder::doesPassTrackSelection(
    const BoundTrackParameters& trk) const -> bool {
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

  // Calculate track density quantities
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

}  // namespace Acts
