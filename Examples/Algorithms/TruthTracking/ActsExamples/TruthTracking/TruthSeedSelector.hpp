// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <cstddef>
#include <limits>
#include <string>

namespace ActsFatras {
class Barcode;
}  // namespace ActsFatras

namespace ActsExamples {
struct AlgorithmContext;

/// Select truth particles to be used as 'seeds' of reconstruction algorithms,
/// e.g. track fitting and track finding.
///
/// This pre-selection could help guarantee quality of the 'seeds', i.e. to
/// avoid empty proto track (no recorded hits for the particle). In addition, it
/// could help save unnecessary reconstruction time. For instance, when
/// investigating performance of CombinatorialKalmanFilter (CKF), we might be
/// interested in its performance for only truth particles with pT and number of
/// recorded hits (on sensitive detectors) safistying provided criteria (input
/// measurements of CKF are still recorded hits from all possible particles).
/// Then we could use particles only satisfying provided criteria as the 'seeds'
/// of CKF instead of handling all the truth particles.
//
class TruthSeedSelector final : public IAlgorithm {
 public:
  struct Config {
    /// The input truth particles that should be used to create proto tracks.
    std::string inputParticles;
    /// The input hit-particles map collection.
    std::string inputMeasurementParticlesMap;
    /// The output proto tracks collection.
    std::string outputParticles;
    /// Maximum distance from the origin in the transverse plane
    double rhoMin = 0.;
    double rhoMax = std::numeric_limits<double>::max();
    /// Minimum/Maximum absolute distance from the origin along z
    double zMin = std::numeric_limits<double>::lowest();
    double zMax = std::numeric_limits<double>::max();
    // Truth particle kinematic cuts
    double phiMin = std::numeric_limits<double>::lowest();
    double phiMax = std::numeric_limits<double>::max();
    double etaMin = std::numeric_limits<double>::lowest();
    double etaMax = std::numeric_limits<double>::max();
    double absEtaMin = std::numeric_limits<double>::lowest();
    double absEtaMax = std::numeric_limits<double>::max();
    double ptMin = 0.0;
    double ptMax = std::numeric_limits<double>::max();
    /// Keep neutral particles
    bool keepNeutral = false;
    /// Requirement on number of recorded hits
    //@TODO: implement detector-specific requirements
    std::size_t nHitsMin = 0;
    std::size_t nHitsMax = std::numeric_limits<std::size_t>::max();
  };

  TruthSeedSelector(const Config& config, Acts::Logging::Level level);

  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  ReadDataHandle<HitParticlesMap> m_inputMeasurementParticlesMap{
      this, "InputMeasurementParticlesMap"};

  WriteDataHandle<SimParticleContainer> m_outputParticles{this,
                                                          "OutputParticles"};
};

}  // namespace ActsExamples
