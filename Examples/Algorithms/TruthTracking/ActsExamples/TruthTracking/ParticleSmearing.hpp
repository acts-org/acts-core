// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"

#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <string>

namespace ActsExamples {
class RandomNumbers;
struct AlgorithmContext;

/// Create track states by smearing truth particle information.
///
/// Particles are smeared in the perigee frame anchored at their true vertex
/// position. The `d0` and `z0` parameters are always defined within that
/// perigee frame and not globally. The generated bound parameters are stored in
/// the same order as the input particles.
class ParticleSmearing final : public IAlgorithm {
 public:
  struct Config {
    /// Input truth particles collection.
    std::string inputParticles;
    /// Output smeared tracks parameters collection.
    std::string outputTrackParameters;

    /// Random numbers service.
    std::shared_ptr<const RandomNumbers> randomNumbers = nullptr;

    // Smearing parameters
    /// Constant term of the d0 resolution.
    double sigmaD0 = 20 * Acts::UnitConstants::um;
    /// Pt-dependent d0 resolution of the form sigma_d0 = A*exp(-1.*abs(B)*pt).
    double sigmaD0PtA = 30 * Acts::UnitConstants::um;
    double sigmaD0PtB = 0.3 / Acts::UnitConstants::GeV;
    /// Constant term of the z0 resolution.
    double sigmaZ0 = 20 * Acts::UnitConstants::um;
    /// Pt-dependent z0 resolution of the form sigma_z0 = A*exp(-1.*abs(B)*pt).
    double sigmaZ0PtA = 30 * Acts::UnitConstants::um;
    double sigmaZ0PtB = 0.3 / Acts::UnitConstants::GeV;
    /// Time resolution.
    double sigmaT0 = 1 * Acts::UnitConstants::ns;
    /// Phi angular resolution.
    double sigmaPhi = 1 * Acts::UnitConstants::degree;
    /// Theta angular resolution.
    double sigmaTheta = 1 * Acts::UnitConstants::degree;
    /// Relative transverse momentum resolution.
    double sigmaPtRel = 0.05;

    /// Optional. Initial sigmas for the track parameters which overwrites the
    /// smearing params if set.
    std::optional<std::array<double, 6>> initialSigmas;
    /// Relative pt resolution used for the initial sigma of q/p.
    double initialSigmaPtRel = 0.1;
    /// Inflate the initial covariance matrix
    std::array<double, 6> initialVarInflation = {1e4, 1e4, 1e4, 1e4, 1e4, 1e4};

    /// Optional particle hypothesis override.
    std::optional<Acts::ParticleHypothesis> particleHypothesis = std::nullopt;
  };

  ParticleSmearing(const Config& config, Acts::Logging::Level level);

  ProcessCode execute(const AlgorithmContext& ctx) const override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};

  WriteDataHandle<TrackParametersContainer> m_outputTrackParameters{
      this, "OutputTrackParameters"};
};

}  // namespace ActsExamples
