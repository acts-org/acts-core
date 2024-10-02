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
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Geant4/PDGtoG4Converter.hpp"

#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "G4ParticleDefinition.hh"

class G4RunManager;

namespace ActsFatras {

/// Handle particle decays using the Geant4 decay models.
class Geant4Decay {
 public:
  using Scalar = Particle::Scalar;

  /// Constructor
  Geant4Decay();

  /// Generate the particle life time.
  ///
  /// @tparam generator_t Type of the random number generator
  /// @param [in, out] generator The random number generator
  /// @param [in] particle The particle that gets a lifetime assigned
  ///
  /// @return Proper time limit of the particle
  template <typename generator_t>
  Scalar generateProperTimeLimit(generator_t& generator,
                                 const Particle& particle) const;

  /// Decay the particle and create the decay products.
  ///
  /// @tparam generator_t Type of the random number generator
  /// @param [in, out] particle The particle that may decay
  ///
  /// @return Vector containing decay products
  template <typename generator_t>
  std::vector<Particle> run(generator_t& generator, Particle& particle) const;

 private:
  /// This function evaluates the decay products of a given particle
  ///
  /// @param [in] parent The decaying particle
  ///
  /// @return Vector containing the decay products
  std::vector<Particle> decayParticle(const Particle& parent) const;

  mutable G4RunManager* m_g4RunManager;  ///< for dummy G4 initialization

  PDGtoG4Converter
      m_pdgToG4Conv;  ///< Handle for converting a PDG ID into a Geant4 particle
};

template <typename generator_t>
Particle::Scalar Geant4Decay::generateProperTimeLimit(
    generator_t& generator, const Particle& particle) const {
  // Get the particle properties
  const Acts::PdgParticle pdgCode = particle.pdg();
  // Keep muons stable
  if (makeAbsolutePdgParticle(pdgCode) == Acts::PdgParticle::eMuon) {
    return std::numeric_limits<Scalar>::infinity();
  }

  // Get the Geant4 particle
  G4ParticleDefinition* pDef = m_pdgToG4Conv.getParticleDefinition(pdgCode);

  // Fast exit if the particle is stable
  if (!pDef || pDef->GetPDGStable()) {
    return std::numeric_limits<Scalar>::infinity();
  }

  // Get average lifetime
  constexpr Scalar convertTime = Acts::UnitConstants::mm / CLHEP::s;
  const Scalar tau = pDef->GetPDGLifeTime() * convertTime;
  // Sample & return the lifetime
  std::uniform_real_distribution<Scalar> uniformDistribution{0., 1.};

  return -tau * std::log(uniformDistribution(generator));
}

template <typename generator_t>
std::vector<Particle> Geant4Decay::run(generator_t& /*generator*/,
                                       Particle& particle) const {
  return decayParticle(particle);
}
}  // namespace ActsFatras
