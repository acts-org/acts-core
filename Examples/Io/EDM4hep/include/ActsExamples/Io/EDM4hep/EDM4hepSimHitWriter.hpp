// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Podio/PodioUtil.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <string>

#include <edm4hep/MCParticleCollection.h>
#include <edm4hep/SimTrackerHitCollection.h>

namespace ActsExamples {

/// Write out a simhit collection to EDM4hep.
///
/// Inpersistent information:
/// - particle ID
/// - after4 momentum
/// - hit index
/// - digitization channel
class EDM4hepSimHitWriter final : public WriterT<SimHitContainer> {
 public:
  struct Config {
    /// Which simulated (truth) hits collection to use.
    std::string inputSimHits;
    /// Which simulated (truth) particle collection to use.
    std::string inputParticles;
    /// WWhere to write the output file to.
    std::string outputPath;
    /// Name of the particle collection in EDM4hep.
    std::string outputParticles = "MCParticles";
    /// Name of the sim tracker hit collection in EDM4hep
    std::string outputSimTrackerHits = "ActsSimTrackerHits";
  };

  /// Construct the cluster writer.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  EDM4hepSimHitWriter(const Config& config, Acts::Logging::Level level);

  ProcessCode finalize() final;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 protected:
  /// Type-specific write implementation.
  ///
  /// @param[in] ctx is the algorithm context
  /// @param[in] simHits are the simhits to be written
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const SimHitContainer& simHits) final;

 private:
  Config m_cfg;

  Acts::PodioUtil::ROOTWriter m_writer;

  std::mutex m_writeMutex;

  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
};

}  // namespace ActsExamples
