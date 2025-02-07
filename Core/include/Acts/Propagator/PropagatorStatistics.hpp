// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <Acts/Propagator/NavigatorStatistics.hpp>
#include <Acts/Propagator/StepperStatistics.hpp>

namespace Acts {

/// @struct PropagatorStatistics
///
/// @brief A struct to hold statistics of the propagator
struct PropagatorStatistics {
  /// Statistics of the stepper
  StepperStatistics stepping;
  /// Statistics of the navigator
  NavigatorStatistics navigation;
};

}  // namespace Acts
