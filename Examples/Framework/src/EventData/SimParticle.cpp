// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/EventData/SimParticle.hpp"

std::ostream& ActsExamples::operator<<(std::ostream& os,
                                       const SimParticle& particle) {
  // compact format w/ only identity information but no kinematics
  os << "id=" << particle.particleId().value() << "(" << particle.particleId()
     << ")";
  os << "|pdg=" << particle.pdg();
  os << "|q=" << particle.charge();
  os << "|m=" << particle.mass();
  os << "|p=" << particle.absoluteMomentum();
  return os;
}
