// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/PropagationSummary.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <string>

namespace ActsExamples {

class PropagatorInterface;
struct AlgorithmContext;

/// @brief This simple algorithm takes the SimHits and input particles
/// and converts them into a PropagationSummary object.
///
/// This allows for simulation and propagation to be compared at
/// the same output format.
class SimHitToSummaryConversion : public IAlgorithm {
 public:
  struct Config {
    /// The input collection
    std::string inputSimHits = "SimHitContainer";

    /// The input particles (for geometric matching)
    std::string inputParticles = "SimParticleContainer";

    /// The step collection to be stored
    std::string outputSummaryCollection = "PropagationSummary";

    /// Map of surface by identifier to allow local - to global
    std::unordered_map<Acts::GeometryIdentifier, const Acts::Surface*>
        surfaceByIdentifier;
  };

  /// Constructor
  /// @param [in] config is the configuration struct
  /// @param [in] loglevel is the logging level
  SimHitToSummaryConversion(const Config& config, Acts::Logging::Level level);

  /// Framework execute method
  /// @param [in] the algorithm context for event consistency
  /// @return is a process code indicating success or not
  ActsExamples::ProcessCode execute(
      const AlgorithmContext& context) const override;

  /// Get const access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  ReadDataHandle<SimHitContainer> m_inputSimHits{this, "SimHitContainer"};
  ReadDataHandle<SimParticleContainer> m_inputParticles{this,
                                                        "SimParticleContainer"};
  WriteDataHandle<PropagationSummaries> m_outputSummary{this, "OutputSummary"};
};

}  // namespace ActsExamples
