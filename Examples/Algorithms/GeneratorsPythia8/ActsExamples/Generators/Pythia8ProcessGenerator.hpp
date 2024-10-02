// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimVertex.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/Generators/EventGenerator.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Pythia8 {
class Pythia;
}

namespace ActsExamples {

struct Pythia8RandomEngineWrapper;

class Pythia8Generator : public EventGenerator::ParticlesGenerator {
 public:
  struct Config {
    /// PDG particle number of the first incoming beam.
    Acts::PdgParticle pdgBeam0 = Acts::PdgParticle::eProton;
    /// PDG particle number of the second incoming beam.
    Acts::PdgParticle pdgBeam1 = Acts::PdgParticle::eProton;
    /// Center-of-mass energy.
    double cmsEnergy = 14 * Acts::UnitConstants::TeV;
    /// Additional Pythia8 settings.
    std::vector<std::string> settings = {{"HardQCD:all = on"}};
    /// Let pythia print summarized event info
    bool printShortEventListing = false;
    /// Let pythia print detailed event info
    bool printLongEventListing = false;
    /// Turn on/off the labeling of secondary vertices
    /// TODO this is essentially broken as the current code will label any kind
    /// of decay as secondary
    bool labelSecondaries = true;
    /// The spatial threshold to consider a particle originating from a vertex
    double spatialVertexThreshold = 1.0 * Acts::UnitConstants::um;
    /// Random seed for the initialization stage of Pythia8
    unsigned int initializationSeed = 42;
  };

  Pythia8Generator(const Config& cfg, Acts::Logging::Level lvl);
  ~Pythia8Generator() override;
  // try to prevent pythia breakage by forbidding copying
  Pythia8Generator() = delete;
  Pythia8Generator(const Pythia8Generator&) = delete;
  Pythia8Generator(Pythia8Generator&&) = delete;
  Pythia8Generator& operator=(const Pythia8Generator&) = delete;
  Pythia8Generator& operator=(Pythia8Generator&& other) = delete;

  std::pair<SimVertexContainer, SimParticleContainer> operator()(
      RandomEngine& rng) override;

 private:
  /// Private access to the logging instance
  const Acts::Logger& logger() const { return (*m_logger); }

  Config m_cfg;
  std::unique_ptr<const Acts::Logger> m_logger;
  std::unique_ptr<::Pythia8::Pythia> m_pythia8;
  std::shared_ptr<Pythia8RandomEngineWrapper> m_pythia8RndmEngine;
  std::mutex m_pythia8Mutex;
};

}  // namespace ActsExamples
