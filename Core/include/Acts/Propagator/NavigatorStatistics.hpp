// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstddef>

namespace Acts {

/// @struct NavigatorStatistics
///
/// @brief A struct to hold statistics of the navigator
struct NavigatorStatistics {
  /// Number of renavigation attempts
  std::size_t nRenavigations = 0;

  /// Number of volume switches
  std::size_t nVolumeSwitches = 0;
};

}  // namespace Acts
