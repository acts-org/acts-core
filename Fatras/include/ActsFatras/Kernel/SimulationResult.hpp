// This file is part of the Acts project.
//
// Copyright (C) 2020-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsFatras/EventData/Hit.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <limits>
#include <vector>

namespace ActsFatras {

/// Single particle simulation result (and intermediate state).
///
/// This result struct is used by multiple components and is thus defined
/// separately from its usage.
struct SimulationResult {
  /// Current/ final particle state.
  Particle particle;
  /// Additional particles generated by interactions or decay.
  std::vector<Particle> generatedParticles;
  /// Hits created by the particle.
  std::vector<Hit> hits;

  // The following variables are internal implementation details that must be
  // defined here for technical reasons.
  //
  // Values are initialized to NaN so the simulation actor can detect when it is
  // called for the first time, i.e. when the result struct is
  // default-initialized.

  // Whether the particle is still alive and the simulation should continue
  bool isAlive = true;
  // Proper time limit before decay.
  Particle::Scalar properTimeLimit =
      std::numeric_limits<Particle::Scalar>::quiet_NaN();
  // Accumulated radiation/interaction length limit before next interaction.
  Particle::Scalar x0Limit = std::numeric_limits<Particle::Scalar>::quiet_NaN();
  Particle::Scalar l0Limit = std::numeric_limits<Particle::Scalar>::quiet_NaN();
  // Process selection for the next interaction.
  std::size_t x0Process = SIZE_MAX;
  std::size_t l0Process = SIZE_MAX;
};

}  // namespace ActsFatras
