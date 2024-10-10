// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/MultiTrajectoryHelpers.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackContainerFrontendConcept.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace Acts {

/// Evicts tracks that seem to be duplicates or fakes. This algorithm takes a
/// greedy approach in the sense that it will remove the track which looks "most
/// duplicate/fake" first and continues the same process with the rest. That
/// process continues until the final state conditions are met.
///
/// The implementation works as follows:
///  1) Calculate shared hits per track.
///  2) If the maximum shared hits criteria is met, we are done.
///     This is the configurable amount of shared hits we are ok with
///     in our experiment.
///  3) Else, remove the track with the highest relative shared hits (i.e.
///     shared hits / hits).
///  4) Back to square 1.
class GreedyAmbiguityResolution {
 public:
  struct Config {
    /// Maximum amount of shared hits per track.
    std::uint32_t maximumSharedHits = 1;
    /// Maximum number of iterations
    std::uint32_t maximumIterations = 1000;

    /// Minimum number of measurement to form a track.
    std::size_t nMeasurementsMin = 7;
  };

  struct State {
    std::size_t numberOfTracks{};

    std::vector<int> trackTips;
    std::vector<float> trackChi2;
    std::vector<std::vector<std::size_t>> measurementsPerTrack;

    // TODO consider boost 1.81 unordered_flat_map
    boost::container::flat_map<std::size_t,
                               boost::container::flat_set<std::size_t>>
        tracksPerMeasurement;
    std::vector<std::size_t> sharedMeasurementsPerTrack;

    // TODO consider boost 1.81 unordered_flat_map
    boost::container::flat_set<std::size_t> selectedTracks;
  };

  GreedyAmbiguityResolution(const Config& cfg,
                            std::unique_ptr<const Logger> logger =
                                getDefaultLogger("GreedyAmbiguityResolution",
                                                 Logging::INFO))
      : m_cfg{cfg}, m_logger{std::move(logger)} {}

  /// Computes the initial state for the input data. This function accumulates
  /// information that will later be used to accelerate the ambiguity
  /// resolution.
  ///
  /// @param tracks The input track container.
  /// @param state An empty state object which is expected to be default constructed.
  /// @param sourceLinkHash A functor to acquire a hash from a given source link.
  /// @param sourceLinkEquality A functor to check equality of two source links.
  template <TrackContainerFrontend track_container_t,
            typename source_link_hash_t, typename source_link_equality_t>
  void computeInitialState(const track_container_t& tracks, State& state,
                           source_link_hash_t&& sourceLinkHash,
                           source_link_equality_t&& sourceLinkEquality) const;

  /// Updates the state iteratively by evicting one track after the other until
  /// the final state conditions are met.
  ///
  /// @param state A state object that was previously filled by the initialization.
  void resolve(State& state) const;

 private:
  Config m_cfg;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to logging instance
  const Logger& logger() const { return *m_logger; }
};

}  // namespace Acts

#include "Acts/AmbiguityResolution/GreedyAmbiguityResolution.ipp"
