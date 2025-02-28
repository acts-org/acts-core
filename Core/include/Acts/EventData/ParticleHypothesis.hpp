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
#include "Acts/EventData/Charge.hpp"
#include "Acts/EventData/GenericParticleHypothesis.hpp"

namespace Acts {

// TODO In principle the factory methods could provide a reference to a static
// instance which would avoid copying the particle hypothesis and potentially
// save some memory. But constexpr+static seems to require C++2b extension.

/// Specialized particle hypothesis for singly charged particles.
///
/// @note This serves as a factory for common singly charge particles.
class SinglyChargedParticleHypothesis
    : public GenericParticleHypothesis<SinglyCharged> {
 public:
  constexpr SinglyChargedParticleHypothesis(PdgParticle absPdg, float mass)
      : GenericParticleHypothesis(absPdg, mass, {}) {}

  explicit SinglyChargedParticleHypothesis(PdgParticle absPdg)
      : GenericParticleHypothesis(absPdg) {}

  template <typename other_charge_t>
  explicit constexpr SinglyChargedParticleHypothesis(
      const GenericParticleHypothesis<other_charge_t>& other)
      : GenericParticleHypothesis(other) {}

  static SinglyChargedParticleHypothesis muon() {
    static const SinglyChargedParticleHypothesis cache(PdgParticle::eMuon);
    return cache;
  }
  static SinglyChargedParticleHypothesis pion() {
    static const SinglyChargedParticleHypothesis cache(PdgParticle::ePionPlus);
    return cache;
  }
  static SinglyChargedParticleHypothesis electron() {
    static const SinglyChargedParticleHypothesis cache(PdgParticle::eElectron);
    return cache;
  }
  static SinglyChargedParticleHypothesis kaon() {
    static const SinglyChargedParticleHypothesis cache(PdgParticle::eKaonPlus);
    return cache;
  }
  static SinglyChargedParticleHypothesis proton() {
    static const SinglyChargedParticleHypothesis cache(PdgParticle::eProton);
    return cache;
  }

  static SinglyChargedParticleHypothesis chargedGeantino() {
    static const SinglyChargedParticleHypothesis cache(PdgParticle::eInvalid,
                                                       0);
    return cache;
  }
};

/// Specialized particle hypothesis for neutral particles.
///
/// @note This serves as a factory for common neutral particles.
class NeutralParticleHypothesis : public GenericParticleHypothesis<Neutral> {
 public:
  constexpr NeutralParticleHypothesis(PdgParticle absPdg, float mass)
      : GenericParticleHypothesis(absPdg, mass, {}) {}
  explicit NeutralParticleHypothesis(PdgParticle absPdg)
      : GenericParticleHypothesis(absPdg) {}

  template <typename other_charge_t>
  explicit constexpr NeutralParticleHypothesis(
      const GenericParticleHypothesis<other_charge_t>& other)
      : GenericParticleHypothesis(other) {}

  static NeutralParticleHypothesis photon() {
    static const NeutralParticleHypothesis cache(PdgParticle::eGamma);
    return cache;
  }
  static NeutralParticleHypothesis pion0() {
    static const NeutralParticleHypothesis cache(PdgParticle::ePionZero);
    return cache;
  }

  static NeutralParticleHypothesis geantino() {
    static const NeutralParticleHypothesis cache(PdgParticle::eInvalid, 0);
    return cache;
  }
};

/// Specialized particle hypothesis for non-neutral particles.
///
/// @note This serves as a factory for common non-neutral particles.
class NonNeutralChargedParticleHypothesis
    : public GenericParticleHypothesis<NonNeutralCharge> {
 public:
  constexpr NonNeutralChargedParticleHypothesis(PdgParticle absPdg, float mass,
                                                NonNeutralCharge chargeType)
      : GenericParticleHypothesis(absPdg, mass, chargeType) {}
  explicit NonNeutralChargedParticleHypothesis(PdgParticle absPdg)
      : GenericParticleHypothesis(absPdg) {}

  template <typename other_charge_t>
  explicit constexpr NonNeutralChargedParticleHypothesis(
      const GenericParticleHypothesis<other_charge_t>& other)
      : GenericParticleHypothesis(other) {}

  static NonNeutralChargedParticleHypothesis muon() {
    return NonNeutralChargedParticleHypothesis{
        SinglyChargedParticleHypothesis::muon()};
  }
  static NonNeutralChargedParticleHypothesis pion() {
    return NonNeutralChargedParticleHypothesis{
        SinglyChargedParticleHypothesis::pion()};
  }
  static NonNeutralChargedParticleHypothesis electron() {
    return NonNeutralChargedParticleHypothesis{
        SinglyChargedParticleHypothesis::electron()};
  }
  static NonNeutralChargedParticleHypothesis kaon() {
    return NonNeutralChargedParticleHypothesis{
        SinglyChargedParticleHypothesis::kaon()};
  }
  static NonNeutralChargedParticleHypothesis proton() {
    return NonNeutralChargedParticleHypothesis{
        SinglyChargedParticleHypothesis::proton()};
  }

  static NonNeutralChargedParticleHypothesis pionLike(float absQ) {
    return NonNeutralChargedParticleHypothesis(
        pion().absolutePdg(), pion().mass(), NonNeutralCharge{absQ});
  }

  static NonNeutralChargedParticleHypothesis chargedGeantino() {
    static const auto cache = chargedGeantino(Acts::UnitConstants::e);
    return cache;
  }
  static NonNeutralChargedParticleHypothesis chargedGeantino(float absQ) {
    return NonNeutralChargedParticleHypothesis(PdgParticle::eInvalid, 0,
                                               NonNeutralCharge{absQ});
  }
};

/// Specialized particle hypothesis for any kind of charged particles.
///
/// @note This serves as a factory for common particles with any kind of charge.
class ParticleHypothesis : public GenericParticleHypothesis<AnyCharge> {
 public:
  constexpr ParticleHypothesis(PdgParticle absPdg, float mass,
                               AnyCharge chargeType)
      : GenericParticleHypothesis(absPdg, mass, chargeType) {}
  explicit ParticleHypothesis(PdgParticle absPdg)
      : GenericParticleHypothesis(absPdg) {}

  template <typename other_charge_t>
  explicit constexpr ParticleHypothesis(
      const GenericParticleHypothesis<other_charge_t>& other)
      : GenericParticleHypothesis(other) {}

  static ParticleHypothesis muon() {
    return ParticleHypothesis{SinglyChargedParticleHypothesis::muon()};
  }
  static ParticleHypothesis pion() {
    return ParticleHypothesis{SinglyChargedParticleHypothesis::pion()};
  }
  static ParticleHypothesis electron() {
    return ParticleHypothesis{SinglyChargedParticleHypothesis::electron()};
  }
  static ParticleHypothesis kaon() {
    return ParticleHypothesis{SinglyChargedParticleHypothesis::kaon()};
  }
  static ParticleHypothesis proton() {
    return ParticleHypothesis{SinglyChargedParticleHypothesis::proton()};
  }

  static ParticleHypothesis photon() {
    return ParticleHypothesis{NeutralParticleHypothesis::photon()};
  }
  static ParticleHypothesis pion0() {
    return ParticleHypothesis{NeutralParticleHypothesis::pion0()};
  }

  static ParticleHypothesis pionLike(float absQ) {
    return ParticleHypothesis(pion().absolutePdg(), pion().mass(),
                              AnyCharge{absQ});
  }

  static ParticleHypothesis geantino() {
    return ParticleHypothesis{NeutralParticleHypothesis::geantino()};
  }
  static ParticleHypothesis chargedGeantino() {
    static const auto cache = chargedGeantino(Acts::UnitConstants::e);
    return cache;
  }
  static ParticleHypothesis chargedGeantino(float absQ) {
    return ParticleHypothesis(PdgParticle::eInvalid, 0, AnyCharge{absQ});
  }
};

}  // namespace Acts
