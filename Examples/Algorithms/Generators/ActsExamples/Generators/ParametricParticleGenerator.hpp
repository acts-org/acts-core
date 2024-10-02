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
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/Generators/EventGenerator.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>

namespace ActsExamples {

/// Generate particles from uniform parameter distributions.
///
/// Generates a single vertex with the given number of tracks. The track
/// direction is drawn from a uniform distribution on the unit sphere (within
/// the given limits). Its absolute momentum is drawn from a uniform
/// distribution. Position and time are always set to zero.
class ParametricParticleGenerator : public EventGenerator::ParticlesGenerator {
 public:
  struct Config {
    /// Low, high (exclusive) for the transverse direction angle.
    double phiMin = -M_PI;
    double phiMax = M_PI;
    /// Low, high (inclusive) for  the longitudinal direction angle.
    ///
    /// This intentionally uses theta instead of eta so it can represent the
    /// full direction space with finite values.
    ///
    /// @note This is the standard generation, for detector performance
    /// classification, where a flat distribution in eta can be useful,
    /// this can be set by the etaUniform flag;
    ///
    double thetaMin = std::numeric_limits<double>::min();
    double thetaMax = M_PI - std::numeric_limits<double>::epsilon();
    bool etaUniform = false;
    /// Low, high (exclusive) for absolute/transverse momentum.
    double pMin = 1 * Acts::UnitConstants::GeV;
    double pMax = 10 * Acts::UnitConstants::GeV;
    /// Indicate if the momentum referse to transverse momentum
    bool pTransverse = false;
    /// (Absolute) PDG particle number to identify the particle type.
    Acts::PdgParticle pdg = Acts::PdgParticle::eMuon;
    /// Randomize the charge and flip the PDG particle number sign accordingly.
    bool randomizeCharge = false;
    /// Number of particles.
    std::size_t numParticles = 1;

    /// Overrides particle charge.
    std::optional<double> charge;
    /// Overrides particle mass.
    std::optional<double> mass;
  };

  ParametricParticleGenerator(const Config& cfg);

  /// Generate a single primary vertex with the given number of particles.
  std::pair<SimVertexContainer, SimParticleContainer> operator()(
      RandomEngine& rng) override;

 private:
  Config m_cfg;
  // will be automatically set from PDG data tables
  double m_charge;
  double m_mass;
  double m_cosThetaMin;
  double m_cosThetaMax;
  double m_etaMin;
  double m_etaMax;
};

}  // namespace ActsExamples
