// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Propagator/MaterialInteractor.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/SequenceElement.hpp"

#include <memory>
#include <mutex>

#include <HepMC3/GenEvent.h>

class G4RunManager;

namespace ActsExamples {

class DetectorConstructionFactory;

class EventRecording final : public ActsExamples::IAlgorithm {
 public:
  /// @class Config
  struct Config {
    /// The input collection of particles
    std::string inputParticles = "";
    /// The recorded events output
    std::string outputHepMcTracks = "geant-outcome-tracks";

    std::shared_ptr<DetectorConstructionFactory> detectorConstructionFactory;

    /// random number seed 1
    int seed1 = 12345;
    /// random number seed 2
    int seed2 = 45678;

    /// List of processes that can be combined to a single vertex
    std::vector<std::string> processesCombine;
    /// Optional selective recording based on a process
    /// @note All events are recorded if this is empty
    std::string processSelect;
    /// List to veto events with certain processes
    std::vector<std::string> processesReject;
  };

  /// Constructor
  /// @param config the configuration
  /// @param level the log level
  EventRecording(const Config& config, Acts::Logging::Level level);

  ~EventRecording() override;

  ActsExamples::ProcessCode execute(
      const AlgorithmContext& context) const override;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  /// The config object
  Config m_cfg;
  /// G4 run manager
  std::unique_ptr<G4RunManager> m_runManager;

  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  WriteDataHandle<std::vector<HepMC3::GenEvent>> m_outputEvents{this,
                                                                "OutputEvents"};

  // has to be mutable; algorithm interface enforces object constness
  mutable std::mutex m_runManagerLock;
};
}  // namespace ActsExamples
