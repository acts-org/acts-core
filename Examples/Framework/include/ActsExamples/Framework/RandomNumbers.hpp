// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

//
//  RandomNumbers.hpp
//  ActsExamples
//
//  Created by Andreas Salzburger on 17/05/16.
//
//

#pragma once

#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <cstdint>
#include <random>

namespace ActsExamples {
struct AlgorithmContext;

/// The random number generator used in the framework.
using RandomEngine = std::mt19937;  ///< Mersenne Twister

/// Provide event and algorithm specific random number generator.s
///
/// This provides local random number generators, allowing for
/// thread-safe, lock-free, and reproducible random number generation across
/// single-threaded and multi-threaded test framework runs.
///
/// The role of the RandomNumbers is only to spawn local random number
/// generators. It does not, in and of itself, accommodate requests for specific
/// random number distributions (uniform, gaussian, etc). For this purpose,
/// clients should spawn their own local distribution objects
/// as needed, following the C++11 STL design.
class RandomNumbers {
 public:
  struct Config {
    std::uint64_t seed = 1234567890u;  ///< random seed
  };

  RandomNumbers(const Config& cfg);

  /// Spawn an algorithm-local random number generator. To avoid inefficiencies
  /// and multiple uses of a given RNG seed, this should only be done once per
  /// Algorithm invocation, after what the generator object should be reused.
  ///
  /// It calls generateSeed() for an event driven seed
  ///
  /// @param context is the AlgorithmContext of the host algorithm
  RandomEngine spawnGenerator(const AlgorithmContext& context) const;

  /// Generate a event and algorithm specific seed value.
  ///
  /// This should only be used in special cases e.g. where a custom
  /// random engine is used and `spawnGenerator` can not be used.
  std::uint64_t generateSeed(const AlgorithmContext& context) const;

 private:
  Config m_cfg;
};

}  // namespace ActsExamples
