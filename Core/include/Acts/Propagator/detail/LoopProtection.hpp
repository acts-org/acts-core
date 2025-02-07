// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <numbers>

namespace Acts::detail {

/// Estimate the loop protection limit
template <typename path_aborter_t, typename propagator_state_t,
          typename stepper_t>
void setupLoopProtection(propagator_state_t& state, const stepper_t& stepper,
                         path_aborter_t& pathAborter, bool releaseLimit,
                         const Logger& logger) {
  if (!state.options.loopProtection) {
    return;
  }

  if (releaseLimit) {
    pathAborter.internalLimit =
        state.options.direction * std::numeric_limits<double>::max();
  }

  // Get the field at the start position
  auto fieldRes =
      stepper.getField(state.stepping, stepper.position(state.stepping));
  if (!fieldRes.ok()) {
    // there's no great way to return the error here, so resort to warning
    // and not applying the loop protection in this case
    ACTS_WARNING("Field lookup was unsuccessful, this is very likely an error");
    return;
  }
  const Vector3 field = *fieldRes;
  const double B = field.norm();
  if (B == 0) {
    return;
  }

  // Transverse component at start is taken for the loop protection
  const double p = stepper.absoluteMomentum(state.stepping);
  // Calculate the full helix path
  const double helixPath =
      state.options.direction * 2 * std::numbers::pi * p / B;
  // And set it as the loop limit if it overwrites the internal limit
  const double loopLimit = state.options.loopFraction * helixPath;
  const double previousLimit = pathAborter.internalLimit;
  if (std::abs(loopLimit) < std::abs(previousLimit)) {
    pathAborter.internalLimit = loopLimit;

    ACTS_VERBOSE("Path aborter limit set to "
                 << loopLimit << " (full helix = " << helixPath
                 << ", previous limit = " << previousLimit << ")");
  } else {
    ACTS_VERBOSE("Path aborter limit not updated to "
                 << loopLimit << " (full helix = " << helixPath
                 << ", previous limit = " << previousLimit << ")");
  }
}

}  // namespace Acts::detail
