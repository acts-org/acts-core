// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/AmbiguityResolution/GreedyAmbiguityResolution.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ActsExamples {
struct AlgorithmContext;

/// Evicts tracks that seem to be duplicated.
///
/// The implementation works as follows:
///  1) Calculate shared hits per track.
///  2) If the maximum shared hits criteria is met, we are done.
///     This is the configurable amount of shared hits we are ok with
///     in our experiment.
///  3) Else, remove the track with the highest relative shared hits (i.e.
///     shared hits / hits).
///  4) Back to square 1.
class GreedyAmbiguityResolutionAlgorithm final : public IAlgorithm {
 public:
  struct Config {
    /// Input trajectories collection.
    std::string inputTracks;
    /// Output trajectories collection.
    std::string outputTracks;

    /// Maximum amount of shared hits per track.
    std::uint32_t maximumSharedHits = 1;
    /// Maximum number of iterations
    std::uint32_t maximumIterations = 10000;

    /// Minimum number of measurement to form a track.
    std::size_t nMeasurementsMin = 7;
  };

  /// Construct the ambiguity resolution algorithm.
  ///
  /// @param cfg is the algorithm configuration
  /// @param lvl is the logging level
  GreedyAmbiguityResolutionAlgorithm(Config cfg, Acts::Logging::Level lvl);

  /// Run the ambiguity resolution algorithm.
  ///
  /// @param cxt is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Const access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  Acts::GreedyAmbiguityResolution m_core;

  ReadDataHandle<ConstTrackContainer> m_inputTracks{this, "InputTracks"};
  WriteDataHandle<ConstTrackContainer> m_outputTracks{this, "OutputTracks"};
};

}  // namespace ActsExamples
