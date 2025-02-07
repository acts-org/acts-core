// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstddef>

namespace Acts {

/// @struct StepperStatistics
///
/// @brief A struct to hold statistics of the stepper
struct StepperStatistics {
  /// Number of attempted steps
  std::size_t nAttemptedSteps = 0;
  /// Number of rejected steps
  std::size_t nRejectedSteps = 0;
  /// Number of successful steps
  std::size_t nSuccessfulSteps = 0;
  /// Number of steps that were reversed
  std::size_t nReverseSteps = 0;

  /// Signed sum of the step lengths
  double pathLength = 0;
  /// Unsigned sum of the step lengths
  double absolutePathLength = 0;
};

}  // namespace Acts
