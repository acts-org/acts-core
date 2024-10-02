// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Propagator/ConstrainedStep.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <limits>

namespace Acts::detail {

/// Update surface status - Single component
///
/// This method intersect the provided surface and update the navigation
/// step estimation accordingly (hence it changes the state). It also
/// returns the status of the intersection to trigger onSurface in case
/// the surface is reached.
///
/// @param state [in,out] The stepping state (thread-local cache)
/// @param surface [in] The surface provided
/// @param boundaryTolerance [in] The boundary check for this status update
template <typename stepper_t>
Acts::Intersection3D::Status updateSingleSurfaceStatus(
    const stepper_t& stepper, typename stepper_t::State& state,
    const Surface& surface, std::uint8_t index, Direction navDir,
    const BoundaryTolerance& boundaryTolerance, ActsScalar surfaceTolerance,
    const Logger& logger) {
  ACTS_VERBOSE("Update single surface status for surface: "
               << surface.geometryId() << " index " << static_cast<int>(index));

  auto sIntersection =
      surface.intersect(state.geoContext, stepper.position(state),
                        navDir * stepper.direction(state), boundaryTolerance,
                        surfaceTolerance)[index];

  // The intersection is on surface already
  if (sIntersection.status() == Intersection3D::Status::onSurface) {
    // Release navigation step size
    state.stepSize.release(ConstrainedStep::actor);
    ACTS_VERBOSE("Intersection: state is ON SURFACE");
    return Intersection3D::Status::onSurface;
  }

  const double nearLimit = std::numeric_limits<double>::lowest();
  const double farLimit = state.stepSize.value(ConstrainedStep::aborter);

  if (sIntersection.isValid() &&
      detail::checkPathLength(sIntersection.pathLength(), nearLimit, farLimit,
                              logger)) {
    ACTS_VERBOSE("Surface is reachable");
    stepper.updateStepSize(state, sIntersection.pathLength(),
                           ConstrainedStep::actor);
    return Intersection3D::Status::reachable;
  }

  ACTS_VERBOSE("Surface is NOT reachable");
  return Intersection3D::Status::unreachable;
}

/// Update the Step size - single component
///
/// It takes a (valid) object intersection from the compatibleX(...)
/// calls in the geometry and updates the step size
///
/// @param state [in,out] The stepping state (thread-local cache)
/// @param oIntersection [in] The object that yielded this step size
/// @param release [in] A release flag
template <typename stepper_t, typename object_intersection_t>
void updateSingleStepSize(typename stepper_t::State& state,
                          const object_intersection_t& oIntersection,
                          bool release = true) {
  double stepSize = oIntersection.pathLength();
  state.stepSize.update(stepSize, ConstrainedStep::actor, release);
}

}  // namespace Acts::detail
