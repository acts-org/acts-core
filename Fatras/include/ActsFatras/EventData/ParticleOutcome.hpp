// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
