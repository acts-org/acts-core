// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Direction.hpp"
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
Acts::IntersectionStatus updateSingleSurfaceStatus(
    const stepper_t& stepper, typename stepper_t::State& state,
    const Surface& surface, std::uint8_t index, Direction navDir,
    const BoundaryTolerance& boundaryTolerance, double surfaceTolerance,
    ConstrainedStep::Type stype, bool release, const Logger& logger) {
  ACTS_VERBOSE("Update single surface status for surface: "
               << surface.geometryId() << " index " << static_cast<int>(index));

  auto sIntersection =
      surface.intersect(state.options.geoContext, stepper.position(state),
                        navDir * stepper.direction(state), boundaryTolerance,
                        surfaceTolerance)[index];

  // The intersection is on surface already
  if (sIntersection.status() == IntersectionStatus::onSurface) {
    ACTS_VERBOSE("Intersection: state is ON SURFACE");
    stepper.updateStepSize(state, sIntersection.pathLength(), stype, release);
    return IntersectionStatus::onSurface;
  }

  const double nearLimit = std::numeric_limits<double>::lowest();
  const double farLimit = std::numeric_limits<double>::max();

  if (sIntersection.isValid() &&
      detail::checkPathLength(sIntersection.pathLength(), nearLimit, farLimit,
                              logger)) {
    ACTS_VERBOSE("Surface is reachable");
    stepper.updateStepSize(state, sIntersection.pathLength(), stype, release);
    return IntersectionStatus::reachable;
  }

  ACTS_VERBOSE("Surface is NOT reachable");
  return IntersectionStatus::unreachable;
}

}  // namespace Acts::detail
