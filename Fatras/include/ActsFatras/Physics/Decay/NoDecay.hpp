// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
  constexpr double generateProperTimeLimit(
      generator_t& /* rng */, const Particle& /* particle */) const {
    return std::numeric_limits<double>::infinity();
  }
  /// Decay the particle without generating any descendant particles.
  template <typename generator_t>
  constexpr std::array<Particle, 0> run(generator_t& /* rng */,
                                        const Particle& /* particle */) const {
    return {};
  }
};

}  // namespace ActsFatras
