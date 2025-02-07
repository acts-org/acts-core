// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
