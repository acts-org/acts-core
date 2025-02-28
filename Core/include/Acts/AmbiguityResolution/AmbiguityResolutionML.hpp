// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/AmbiguityResolution/AmbiguityNetworkConcept.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace Acts {

/// Generic implementation of the machine learning ambiguity resolution
/// Contains method for data preparations
template <AmbiguityNetworkConcept AmbiguityNetwork>
class AmbiguityResolutionML {
 public:
  struct Config {
    /// Path to the model file for the duplicate neural network
    std::string inputDuplicateNN = "";
    /// Minimum number of measurement to form a track.
    std::size_t nMeasurementsMin = 7;
  };
  /// Construct the ambiguity resolution algorithm.
  ///
  /// @param cfg is the algorithm configuration
  /// @param logger is the logging instance
  explicit AmbiguityResolutionML(const Config& cfg,
                                 std::unique_ptr<const Logger> logger =
                                     getDefaultLogger("AmbiguityResolutionML",
                                                      Logging::INFO))
      : m_cfg{cfg},
        m_duplicateClassifier(m_cfg.inputDuplicateNN.c_str()),
        m_logger{std::move(logger)} {}

  /// Associate the hits to the tracks
  ///
  /// This algorithm performs the mapping of hits ID to track ID. Our final goal
  /// is too loop over all the tracks (and their associated hits) by order of
  /// decreasing number hits for this we use a multimap where the key is the
  /// number of hits as this will automatically perform the sorting.
  ///
  /// @param tracks is the input track container
  /// @param sourceLinkHash is the hash function for the source link, will be used to associate to tracks
  /// @param sourceLinkEquality is the equality function for the source link used used to associated hits to tracks
  /// @return an ordered list containing pairs of track ID and associated measurement ID
  template <TrackContainerFrontend track_container_t,
            typename source_link_hash_t, typename source_link_equality_t>
  std::multimap<int, std::pair<std::size_t, std::vector<std::size_t>>>
  mapTrackHits(const track_container_t& tracks,
               const source_link_hash_t& sourceLinkHash,
               const source_link_equality_t& sourceLinkEquality) const {
    // A map to store (and generate) the measurement index for each source link
    auto measurementIndexMap =
        std::unordered_map<SourceLink, std::size_t, source_link_hash_t,
                           source_link_equality_t>(0, sourceLinkHash,
                                                   sourceLinkEquality);

    // A map to store the track Id and their associated measurements ID, a
    // multimap is used to automatically sort the tracks by the number of
    // measurements
    std::multimap<int, std::pair<std::size_t, std::vector<std::size_t>>>
        trackMap;
    std::size_t trackIndex = 0;
    std::vector<std::size_t> measurements;
    // Loop over all the trajectories in the events
    for (const auto& track : tracks) {
      // Kick out tracks that do not fulfill our initial requirements
      if (track.nMeasurements() < m_cfg.nMeasurementsMin) {
        continue;
      }
      measurements.clear();
      for (auto ts : track.trackStatesReversed()) {
        if (ts.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
          SourceLink sourceLink = ts.getUncalibratedSourceLink();
          // assign a new measurement index if the source link was not seen yet
          auto emplace = measurementIndexMap.try_emplace(
              sourceLink, measurementIndexMap.size());
          measurements.push_back(emplace.first->second);
        }
      }
      trackMap.emplace(track.nMeasurements(),
                       std::make_pair(trackIndex, measurements));
      ++trackIndex;
    }
    return trackMap;
  }

  /// Select the track associated with each cluster
  ///
  /// In this algorithm the call the neural network to score the tracks and then
  /// select the track with the highest score in each cluster
  ///
  /// @param clusters is a map of clusters, each cluster correspond to a vector of track ID
  /// @param tracks is the input track container
  /// @return a vector of trackID corresponding tho the good tracks
  template <TrackContainerFrontend track_container_t>
  std::vector<std::size_t> solveAmbiguity(
      std::unordered_map<std::size_t, std::vector<std::size_t>>& clusters,
      const track_container_t& tracks) const {
    std::vector<std::vector<float>> outputTensor =
        m_duplicateClassifier.inferScores(clusters, tracks);
    std::vector<std::size_t> goodTracks =
        m_duplicateClassifier.trackSelection(clusters, outputTensor);

    return goodTracks;
  }

 private:
  // Configuration
  Config m_cfg;

  // The neural network for duplicate classification, the network
  // implementation is chosen with the AmbiguityNetwork template parameter
  AmbiguityNetwork m_duplicateClassifier;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger = nullptr;

  /// Private access to logging instance
  const Logger& logger() const { return *m_logger; }
};

}  // namespace Acts
