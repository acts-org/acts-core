// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/TrackFinding/TrackSelector.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"

#include <cstddef>
#include <limits>
#include <string>

namespace ActsExamples {

/// Select tracks by applying some selection cuts.
class HitSelector final : public IAlgorithm {
 public:
  struct Config {
    /// Input track collection.
    std::string inputHits;
    /// Output track collection
    std::string outputHits;

    /// Time cut
    double maxTime = std::numeric_limits<double>::max();
  };

  HitSelector(const Config& config, Acts::Logging::Level level);

  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<SimHitContainer> m_inputHits{this, "InputHits"};
  WriteDataHandle<SimHitContainer> m_outputHits{this, "OutputHits"};
};

}  // namespace ActsExamples
