// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
