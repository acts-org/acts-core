// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/MagneticField/MagneticField.hpp"
#include "ActsFatras/Physics/NuclearInteraction/NuclearInteraction.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace Acts {
class MagneticFieldProvider;
class TrackingGeometry;
}  // namespace Acts

namespace ActsExamples {
class RandomNumbers;
struct AlgorithmContext;

namespace detail {
struct FatrasSimulation;
}

/// Fast track simulation using the Acts propagation and navigation.
class FatrasSimulation final : public IAlgorithm {
 public:
  struct Config {
    /// The particles input collection.
    std::string inputParticles;
    /// The simulated particles initial state collection.
    std::string outputParticlesInitial;
    /// The simulated particles final state collection.
    std::string outputParticlesFinal;
    /// The simulated hits output collection.
    std::string outputSimHits;
    /// Parametrisation of nuclear interaction
    std::string imputParametrisationNuclearInteraction =
        "nuclearInteractionParameters";
    /// Random number service.
    std::shared_ptr<const RandomNumbers> randomNumbers;
    /// The tracking geometry that should be used.
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry;
    /// The magnetic field that should be used.
    std::shared_ptr<const Acts::MagneticFieldProvider> magneticField;

    // tuning parameters
    /// Minimal absolute momentum for particles to be simulated.
    double pMin = 0.5 * Acts::UnitConstants::GeV;
    /// Simulate (multiple) scattering for charged particles.
    bool emScattering = true;
    /// Simulate ionisiation/excitation energy loss of charged particles.
    bool emEnergyLossIonisation = true;
    /// Simulate radiative energy loss of charged particles.
    bool emEnergyLossRadiation = true;
    /// Simulate electron-positron pair production by photon conversion.
    bool emPhotonConversion = true;
    /// Generate simulation hits on sensitive surfaces.
    bool generateHitsOnSensitive = true;
    /// Generate simulation hits on surfaces with associated material.
    bool generateHitsOnMaterial = false;
    /// Generate simulation hits on passive surfaces, i.e neither sensitive nor
    /// have associated material.
    bool generateHitsOnPassive = false;

    /// Absolute maximum step size
    double maxStepSize = 1 * Acts::UnitConstants::m;
    /// Absolute maximum path length
    double pathLimit = 30 * Acts::UnitConstants::m;

    /// Expected average number of hits generated per particle.
    ///
    /// This is just a performance optimization hint and has no impact on the
    /// algorithm function. It is used to guess the amount of memory to
    /// pre-allocate to avoid allocation during event simulation.
    std::size_t averageHitsPerParticle = 16u;
  };

  /// Construct the algorithm from a config.
  ///
  /// @param cfg is the configuration struct
  /// @param lvl is the logging level
  FatrasSimulation(Config cfg, Acts::Logging::Level lvl);
  ~FatrasSimulation() override;

  /// Run the simulation for a single event.
  ///
  /// @param ctx the algorithm context containing all event information
  ActsExamples::ProcessCode execute(const AlgorithmContext& ctx) const override;

  /// Const access to the config
  const Config& config() const { return m_cfg; }

  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};

  WriteDataHandle<SimHitContainer> m_outputSimHits{this, "OutputSimHits"};

  WriteDataHandle<SimParticleContainer> m_outputParticlesInitial{
      this, "OutputParticlesInitial"};
  WriteDataHandle<SimParticleContainer> m_outputParticlesFinal{
      this, "OutputParticlesFinal"};

 private:
  Config m_cfg;
  std::unique_ptr<detail::FatrasSimulation> m_sim;
};

}  // namespace ActsExamples
