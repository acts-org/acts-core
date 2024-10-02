// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/AmbiguityResolution/GreedyAmbiguityResolution.hpp"

namespace Acts {

namespace {

/// Removes a track from the state which has to be done for multiple properties
/// because of redundancy.
static void removeTrack(GreedyAmbiguityResolution::State& state,
                        std::size_t iTrack) {
  for (auto iMeasurement : state.measurementsPerTrack[iTrack]) {
    state.tracksPerMeasurement[iMeasurement].erase(iTrack);

    if (state.tracksPerMeasurement[iMeasurement].size() == 1) {
      auto jTrack = *state.tracksPerMeasurement[iMeasurement].begin();
      --state.sharedMeasurementsPerTrack[jTrack];
    }
  }

  state.selectedTracks.erase(iTrack);
}

}  // namespace

void GreedyAmbiguityResolution::resolve(State& state) const {
  /// Compares two tracks based on the number of shared measurements in order to
  /// decide if we already met the final state.
  auto sharedMeasurementsComperator = [&state](std::size_t a, std::size_t b) {
    return state.sharedMeasurementsPerTrack[a] <
           state.sharedMeasurementsPerTrack[b];
  };

  /// Compares two tracks in order to find the one which should be evicted.
  /// First we compare the relative amount of shared measurements. If that is
  /// indecisive we use the chi2.
  auto trackComperator = [&state](std::size_t a, std::size_t b) {
    /// Helper to calculate the relative amount of shared measurements.
    auto relativeSharedMeasurements = [&state](std::size_t i) {
      return 1.0 * state.sharedMeasurementsPerTrack[i] /
             state.measurementsPerTrack[i].size();
    };

    if (relativeSharedMeasurements(a) != relativeSharedMeasurements(b)) {
      return relativeSharedMeasurements(a) < relativeSharedMeasurements(b);
    }
    return state.trackChi2[a] < state.trackChi2[b];
  };

  for (std::size_t i = 0; i < m_cfg.maximumIterations; ++i) {
    // Lazy out if there is nothing to filter on.
    if (state.selectedTracks.empty()) {
      ACTS_VERBOSE("no tracks left - exit loop");
      break;
    }

    // Find the maximum amount of shared measurements per track to decide if we
    // are done or not.
    auto maximumSharedMeasurements = *std::max_element(
        state.selectedTracks.begin(), state.selectedTracks.end(),
        sharedMeasurementsComperator);
    ACTS_VERBOSE(
        "maximum shared measurements "
        << state.sharedMeasurementsPerTrack[maximumSharedMeasurements]);
    if (state.sharedMeasurementsPerTrack[maximumSharedMeasurements] <
        m_cfg.maximumSharedHits) {
      break;
    }

    // Find the "worst" track by comparing them to each other
    auto badTrack =
        *std::max_element(state.selectedTracks.begin(),
                          state.selectedTracks.end(), trackComperator);
    ACTS_VERBOSE("remove track "
                 << badTrack << " nMeas "
                 << state.measurementsPerTrack[badTrack].size() << " nShared "
                 << state.sharedMeasurementsPerTrack[badTrack] << " chi2 "
                 << state.trackChi2[badTrack]);
    removeTrack(state, badTrack);
  }
}

}  // namespace Acts
