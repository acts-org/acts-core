// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <iosfwd>

namespace ActsFatras {

/// Particle outcome identifier.
///
/// Encodes the outcome of the particle after the simulation
enum class ParticleOutcome : std::uint32_t {
  Alive = 0,
  KilledInteraction = 1,
  KilledVolumeExit = 2,
  KilledTime = 3,
  KilledSecondaryParticle = 4,
};

std::ostream &operator<<(std::ostream &os, ParticleOutcome outcome);

}  // namespace ActsFatras
