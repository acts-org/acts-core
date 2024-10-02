// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/ExtractedSimulationProcess.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <memory>
#include <string>
#include <vector>

#include <HepMC3/GenEvent.h>

class G4RunManager;

namespace ActsExamples {

/// @brief This class extracts a certain process from a HepMC event record.
class HepMCProcessExtractor final : public ActsExamples::IAlgorithm {
 public:
  /// @class Config
  struct Config {
    /// The input collection
    std::string inputEvents;
    /// The output collection
    std::string outputSimulationProcesses = "event-fraction";
    /// The process that should be extracted
    std::string extractionProcess;

    /// Minimum absolute value of considered PDG IDs
    int absPdgMin = 40;
    /// Maximum absolute value of considered PDG IDs
    int absPdgMax = 2212;
    /// Minimum momentum of considered particles
    double pMin = 50. * Acts::UnitConstants::MeV;
  };

  /// Constructor
  /// @param config the configuration
  /// @param level the log level
  HepMCProcessExtractor(Config config, Acts::Logging::Level level);
  ~HepMCProcessExtractor() override;

  ActsExamples::ProcessCode execute(
      const AlgorithmContext& context) const override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  /// The config object
  Config m_cfg;

  ReadDataHandle<std::vector<HepMC3::GenEvent>> m_inputEvents{this,
                                                              "InputEvents"};
  WriteDataHandle<ActsExamples::ExtractedSimulationProcessContainer>
      m_outputSimulationProcesses{this, "OutputSimulationProcesses"};
};

}  // namespace ActsExamples
