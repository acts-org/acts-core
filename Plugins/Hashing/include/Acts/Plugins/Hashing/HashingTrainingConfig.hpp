// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>

namespace Acts {

struct HashingTrainingConfig {
  /// Random seed for Annoy
  unsigned int annoySeed = 123456789;

  /// Number of features to use
  std::int32_t f = 1;
};

}  // namespace Acts
