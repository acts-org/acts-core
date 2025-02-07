// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsFatras/EventData/Particle.hpp"

#include "Acts/Definitions/ParticleData.hpp"
#include "Acts/Utilities/MultiIndex.hpp"

#include <ostream>

ActsFatras::Particle::Particle(Barcode particleId, Acts::PdgParticle pdg)
    : Particle(particleId, pdg, findCharge(pdg).value_or(0),
               findMass(pdg).value_or(0)) {}

std::ostream& ActsFatras::operator<<(std::ostream& os,
                                     const ActsFatras::Particle& particle) {
  // compact format w/ only identity information but no kinematics
  os << "id=" << particle.particleId().value() << "(" << particle.particleId()
     << ")";
  os << "|pdg=" << particle.pdg();
  os << "|q=" << particle.charge();
  os << "|m=" << particle.mass();
  os << "|p=" << particle.absoluteMomentum();
  return os;
}
