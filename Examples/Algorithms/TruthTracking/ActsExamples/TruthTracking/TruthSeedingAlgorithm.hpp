// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ActsFatras {
class Barcode;
}  // namespace ActsFatras

namespace Acts {
class TrackingGeometry;
}

namespace ActsExamples {
struct AlgorithmContext;

/// Construct track seeds from particles.
class TruthSeedingAlgorithm final : public IAlgorithm {
 public:
  struct Config {
    /// The input truth particles that should be used for truth seeding.
    std::string inputParticles;
    /// The input hit-particles map collection.
    std::string inputMeasurementParticlesMap;
    /// Input space point collections.
    ///
    /// We allow multiple space point collections to allow different parts of
    /// the detector to use different algorithms for space point construction,
    /// e.g. single-hit space points for pixel-like detectors or double-hit
    /// space points for strip-like detectors.
    std::vector<std::string> inputSpacePoints;
    /// Output successfully seeded truth particles.
    std::string outputParticles;
    /// Output seed collection.
    std::string outputSeeds;
    /// Output proto track collection.
    std::string outputProtoTracks;
    /// Minimum deltaR between space points in a seed
    float deltaRMin = 1. * Acts::UnitConstants::mm;
    /// Maximum deltaR between space points in a seed
    float deltaRMax = 100. * Acts::UnitConstants::mm;
  };

  /// Construct the truth seeding algorithm.
  ///
  /// @param cfg is the algorithm configuration
  /// @param lvl is the logging level
  TruthSeedingAlgorithm(Config cfg, Acts::Logging::Level lvl);

  /// Run the truth seeding algorithm.
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const override;

  /// Const access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  ReadDataHandle<HitParticlesMap> m_inputMeasurementParticlesMap{
      this, "InputMeasurementParticlesMaps"};
  std::vector<std::unique_ptr<ReadDataHandle<SimSpacePointContainer>>>
      m_inputSpacePoints{};

  WriteDataHandle<SimParticleContainer> m_outputParticles{this,
                                                          "OutputParticles"};
  WriteDataHandle<ProtoTrackContainer> m_outputProtoTracks{this,
                                                           "OutputProtoTracks"};
  WriteDataHandle<SimSeedContainer> m_outputSeeds{this, "OutputSeeds"};
};

}  // namespace ActsExamples
