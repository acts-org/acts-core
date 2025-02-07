// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/TrackFinding/TrackSelector.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <cstddef>
#include <limits>
#include <string>

namespace ActsExamples {
struct AlgorithmContext;

/// Select tracks by applying some selection cuts.
class TrackSelectorAlgorithm final : public IAlgorithm {
 public:
  struct Config {
    /// Input track collection.
    std::string inputTracks;
    /// Output track collection
    std::string outputTracks;

    Acts::TrackSelector::Config selectorConfig;
  };

  TrackSelectorAlgorithm(const Config& config, Acts::Logging::Level level);

  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  Acts::TrackSelector m_selector;

  ReadDataHandle<ConstTrackContainer> m_inputTrackContainer{this,
                                                            "InputTracks"};
  WriteDataHandle<ConstTrackContainer> m_outputTrackContainer{this,
                                                              "OutputTracks"};
};

}  // namespace ActsExamples
