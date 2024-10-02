// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/AmbiguityResolution/GreedyAmbiguityResolution.hpp"
#include "Acts/EventData/TrackStateType.hpp"

#include <unordered_map>

namespace Acts {

template <TrackContainerFrontend track_container_t, typename source_link_hash_t,
          typename source_link_equality_t>
void GreedyAmbiguityResolution::computeInitialState(
    const track_container_t& tracks, State& state,
    source_link_hash_t&& sourceLinkHash,
    source_link_equality_t&& sourceLinkEquality) const {
  auto measurementIndexMap =
      std::unordered_map<SourceLink, std::size_t, source_link_hash_t,
                         source_link_equality_t>(0, sourceLinkHash,
                                                 sourceLinkEquality);

  // Iterate through all input tracks, collect their properties like measurement
  // count and chi2 and fill the measurement map in order to relate tracks to
  // each other if they have shared hits.
  for (const auto& track : tracks) {
    // Kick out tracks that do not fulfill our initial requirements
    if (track.nMeasurements() < m_cfg.nMeasurementsMin) {
      continue;
    }
    std::vector<std::size_t> measurements;
    for (auto ts : track.trackStatesReversed()) {
      bool isMeasurement = ts.typeFlags().test(TrackStateFlag::MeasurementFlag);
      bool isOutlier = ts.typeFlags().test(TrackStateFlag::OutlierFlag);
      if (isMeasurement && !isOutlier) {
        SourceLink sourceLink = ts.getUncalibratedSourceLink();
        // assign a new measurement index if the source link was not seen yet
        auto emplace = measurementIndexMap.try_emplace(
            sourceLink, measurementIndexMap.size());
        measurements.push_back(emplace.first->second);
      }
    }

    state.trackTips.push_back(track.index());
    state.trackChi2.push_back(track.chi2() / track.nDoF());
    state.measurementsPerTrack.push_back(std::move(measurements));
    state.selectedTracks.insert(state.numberOfTracks);

    ++state.numberOfTracks;
  }

  // Now we relate measurements to tracks
  for (std::size_t iTrack = 0; iTrack < state.numberOfTracks; ++iTrack) {
    for (auto iMeasurement : state.measurementsPerTrack[iTrack]) {
      state.tracksPerMeasurement[iMeasurement].insert(iTrack);
    }
  }

  // Finally, we can accumulate the number of shared measurements per track
  state.sharedMeasurementsPerTrack =
      std::vector<std::size_t>(state.trackTips.size(), 0);
  for (std::size_t iTrack = 0; iTrack < state.numberOfTracks; ++iTrack) {
    for (auto iMeasurement : state.measurementsPerTrack[iTrack]) {
      if (state.tracksPerMeasurement[iMeasurement].size() > 1) {
        ++state.sharedMeasurementsPerTrack[iTrack];
      }
    }
  }
}

}  // namespace Acts
