// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsFatras/EventData/Particle.hpp"

#include <array>
#include <limits>

namespace ActsFatras {

/// Decay module that treats all particles as stable.
struct NoDecay {
  /// Generate the proper time limit for a particle.
  ///
  /// @returns Always returns infinity as limit.
  template <typename generator_t>
  constexpr Particle::Scalar generateProperTimeLimit(
      generator_t& /* rng */, const Particle& /* particle */) const {
    return std::numeric_limits<Particle::Scalar>::infinity();
  }
  /// Decay the particle without generating any descendant particles.
  template <typename generator_t>
  constexpr std::array<Particle, 0> run(generator_t& /* rng */,
                                        const Particle& /* particle */) const {
    return {};
  }
};

}  // namespace ActsFatras
