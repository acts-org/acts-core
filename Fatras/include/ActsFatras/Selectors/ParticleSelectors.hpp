// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/PdgParticle.hpp"
#include "ActsFatras/EventData/Particle.hpp"

namespace ActsFatras {

/// No-op particle selector that selects all particles.
struct EveryParticle {
  bool operator()(const Particle & /*particle*/) const { return true; }
};

/// Select neutral particles.
struct NeutralSelector {
  bool operator()(const Particle &particle) const {
    return (particle.charge() == 0.);
  }
};

/// Select all charged particles.
struct ChargedSelector {
  bool operator()(const Particle &particle) const {
    return (particle.charge() != 0.);
  }
};

/// Select positively charged particles.
struct PositiveSelector {
  bool operator()(const Particle &particle) const {
    return (0. < particle.charge());
  }
};

/// Select negatively charged particles.
struct NegativeSelector {
  bool operator()(const Particle &particle) const {
    return (particle.charge() < 0.);
  }
};

/// Select particles of one specific type.
///
/// Particle and Antiparticle are treated as two separate types.
template <Acts::PdgParticle Pdg>
struct PdgSelector {
  bool operator()(const Particle &particle) const {
    return (particle.pdg() == Pdg);
  }
};

/// Select particles and antiparticles of one specific type.
template <Acts::PdgParticle Pdg>
struct AbsPdgSelector {
  bool operator()(const Particle &particle) const {
    return (makeAbsolutePdgParticle(particle.pdg()) ==
            makeAbsolutePdgParticle(Pdg));
  }
};

/// Select all particles except one specific type.
///
/// Particle and Antiparticle are treated as two separate types.
template <Acts::PdgParticle Pdg>
struct PdgExcluder {
  bool operator()(const Particle &particle) const {
    return (particle.pdg() != Pdg);
  }
};

/// Select all particles except for (anti-)particles of one specific type.
template <Acts::PdgParticle Pdg>
struct AbsPdgExcluder {
  bool operator()(const Particle &particle) const {
    return (makeAbsolutePdgParticle(particle.pdg()) !=
            makeAbsolutePdgParticle(Pdg));
  }
};

}  // namespace ActsFatras
