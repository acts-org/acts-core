// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"

#include <cstdint>
#include <limits>

namespace Acts {

/// defaults experimental cuts to no operation in both seeding algorithms
inline bool noopExperimentCuts(float /*bottomRadius*/, float /*cotTheta*/) {
  return true;
}

/// @brief Contains parameters for quality seed confirmation
/// @note Requirements on the number of compatible space-points and impact parameters can be defined
/// for different (r, z) regions of the detector (e.g. forward or central
/// region) by SeedConfirmationRange. Seeds are classified as "high-quality"
/// seeds and normal quality seeds. Normal quality seeds are only selected if
/// no other "high-quality" seed has been found for that inner-middle doublet.
/// For optimization reasons, the algorithm only calls the seed confirmation
/// for a certain inner-middle doublet, in case a configurable minimum number
/// of inner-middle-outer triplets have been found.
struct SeedConfirmationRangeConfig {
  /// Minimum z position of middle component of the seed used to
  /// split the region of the detector for seed confirmation
  float zMinSeedConf =
      std::numeric_limits<float>::lowest();  // Acts::UnitConstants::mm

  /// Maximum z position of middle component of the seed used to
  /// split the region of the detector for seed confirmation
  float zMaxSeedConf =
      std::numeric_limits<float>::max();  // Acts::UnitConstants::mm

  /// Radius position of inner seed component that is used to
  /// split the region of the detector for seed confirmation
  float rMaxSeedConf =
      std::numeric_limits<float>::max();  // Acts::UnitConstants::mm

  /// Minimum number of compatible outer space-points required in quality seed
  /// confirmation if inner space-points radius is larger than rMaxSeedConf
  std::size_t nTopForLargeR = 0;
  /// Minimum number of compatible outer space-points required in quality seed
  /// confirmation if inner space-points radius is smaller than rMaxSeedConf
  std::size_t nTopForSmallR = 0;

  /// Minimum radius for inner seed component required in quality seed
  /// confirmation
  float seedConfMinBottomRadius = 60. * Acts::UnitConstants::mm;
  /// Maximum longitudinal impact parameter of seed required in quality seed
  /// confirmation
  float seedConfMaxZOrigin = 150. * Acts::UnitConstants::mm;
  /// Minimum impact parameter of seed required in quality seed confirmation
  float minImpactSeedConf = 1. * Acts::UnitConstants::mm;
};

}  // namespace Acts
