// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsFatras/EventData/ParticleOutcome.hpp"

#include <ostream>
#include <stdexcept>

namespace ActsFatras {

std::ostream &operator<<(std::ostream &os, ParticleOutcome outcome) {
  switch (outcome) {
    case ActsFatras::ParticleOutcome::Alive:
      return (os << "Alive");
    case ActsFatras::ParticleOutcome::KilledInteraction:
      return (os << "KilledInteraction");
    case ActsFatras::ParticleOutcome::KilledVolumeExit:
      return (os << "KilledVolumeExit");
    case ActsFatras::ParticleOutcome::KilledTime:
      return (os << "KilledTime");
    case ActsFatras::ParticleOutcome::KilledSecondaryParticle:
      return (os << "KilledSecondaryParticle");
  }

  throw std::runtime_error("Unknown ParticleOutcome");
}

}  // namespace ActsFatras
