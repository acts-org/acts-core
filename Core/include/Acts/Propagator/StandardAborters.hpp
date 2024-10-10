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
#include "Acts/Definitions/Units.hpp"
#include "Acts/Propagator/ConstrainedStep.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <limits>
#include <optional>
#include <sstream>
#include <string>

namespace Acts {

/// This is the condition that the pathLimit has been reached
struct PathLimitReached {
  /// Boolean switch for Loop protection
  double internalLimit = std::numeric_limits<double>::max();

  /// boolean operator for abort condition without using the result
  ///
  /// @tparam propagator_state_t Type of the propagator state
  /// @tparam stepper_t Type of the stepper
  /// @tparam navigator_t Type of the navigator
  ///
  /// @param [in,out] state The propagation state object
  /// @param [in] stepper Stepper used for propagation
  /// @param [in] navigator Navigator used for propagation
  /// @param logger a logger instance
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  bool checkAbort(propagator_state_t& state, const stepper_t& stepper,
                  const navigator_t& navigator, const Logger& logger) const {
    (void)navigator;

    // Check if the maximum allowed step size has to be updated
    double distance =
        std::abs(internalLimit) - std::abs(state.stepping.pathAccumulated);
    double tolerance = state.options.surfaceTolerance;
    bool limitReached = (std::abs(distance) < std::abs(tolerance));
    if (limitReached) {
      ACTS_VERBOSE("PathLimit aborter | " << "Path limit reached at distance "
                                          << distance);
      return true;
    }
    stepper.updateStepSize(state.stepping, distance, ConstrainedStep::aborter,
                           false);
    ACTS_VERBOSE("PathLimit aborter | "
                 << "Target stepSize (path limit) updated to "
                 << stepper.outputStepSize(state.stepping));
    return false;
  }
};

/// This is the condition that the Surface has been reached it then triggers a
/// propagation abort
struct SurfaceReached {
  const Surface* surface = nullptr;
  BoundaryTolerance boundaryTolerance = BoundaryTolerance::None();

  // TODO https://github.com/acts-project/acts/issues/2738
  /// Distance limit to discard intersections "behind us"
  /// @note this is only necessary because some surfaces have more than one
  ///       intersection
  double nearLimit = -100 * UnitConstants::um;

  SurfaceReached() = default;
  explicit SurfaceReached(double nLimit) : nearLimit(nLimit) {}

  /// boolean operator for abort condition without using the result
  ///
  /// @tparam propagator_state_t Type of the propagator state
  /// @tparam stepper_t Type of the stepper
  /// @tparam navigator_t Type of the navigator
  ///
  /// @param [in,out] state The propagation state object
  /// @param [in] stepper Stepper used for propagation
  /// @param [in] navigator Navigator used for propagation
  /// @param logger a logger instance
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  bool checkAbort(propagator_state_t& state, const stepper_t& stepper,
                  const navigator_t& navigator, const Logger& logger) const {
    if (surface == nullptr) {
      ACTS_VERBOSE("SurfaceReached aborter | Target surface not set.");
      return false;
    }

    if (navigator.currentSurface(state.navigation) == surface) {
      ACTS_VERBOSE("SurfaceReached aborter | Target surface reached.");
      return true;
    }

    // not using the stepper overstep limit here because it does not always work
    // for perigee surfaces
    // note: the near limit is necessary for surfaces with more than one
    // intersection in order to discard the ones which are behind us
    const double farLimit = std::numeric_limits<double>::max();
    const double tolerance = state.options.surfaceTolerance;

    const auto sIntersection = surface->intersect(
        state.geoContext, stepper.position(state.stepping),
        state.options.direction * stepper.direction(state.stepping),
        boundaryTolerance, tolerance);
    const auto closest = sIntersection.closest();

    bool reached = false;

    if (closest.status() == Intersection3D::Status::onSurface) {
      const double distance = closest.pathLength();
      ACTS_VERBOSE(
          "SurfaceReached aborter | "
          "Target surface reached at distance (tolerance) "
          << distance << " (" << tolerance << ")");
      reached = true;
    }

    bool intersectionFound = false;

    for (const auto& intersection : sIntersection.split()) {
      if (intersection.isValid() &&
          detail::checkPathLength(intersection.pathLength(), nearLimit,
                                  farLimit, logger)) {
        stepper.updateStepSize(state.stepping, intersection.pathLength(),
                               ConstrainedStep::aborter, false);
        ACTS_VERBOSE(
            "SurfaceReached aborter | "
            "Target stepSize (surface) updated to "
            << stepper.outputStepSize(state.stepping));
        intersectionFound = true;
        break;
      }
    }

    if (!intersectionFound) {
      ACTS_VERBOSE(
          "SurfaceReached aborter | "
          "Target intersection not found. Maybe next time?");
    }
    return reached;
  }
};

/// Similar to SurfaceReached, but with an infinite overstep limit.
///
/// This can be used to force the propagation to the target surface.
struct ForcedSurfaceReached : SurfaceReached {
  ForcedSurfaceReached()
      : SurfaceReached(std::numeric_limits<double>::lowest()) {}
};

/// This is the condition that the end of world has been reached
/// it then triggers an propagation abort
struct EndOfWorldReached {
  /// boolean operator for abort condition without using the result
  ///
  /// @tparam propagator_state_t Type of the propagator state
  /// @tparam navigator_t Type of the navigator
  ///
  /// @param [in,out] state The propagation state object
  /// @param [in] navigator The navigator object
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  bool checkAbort(propagator_state_t& state, const stepper_t& /*stepper*/,
                  const navigator_t& navigator,
                  const Logger& /*logger*/) const {
    bool endOfWorld = navigator.endOfWorldReached(state.navigation);
    return endOfWorld;
  }
};

/// This is the condition that the end of world has been reached
/// it then triggers a propagation abort
struct VolumeConstraintAborter {
  /// boolean operator for abort condition without using the result
  ///
  /// @tparam propagator_state_t Type of the propagator state
  /// @tparam navigator_t Type of the navigator
  ///
  /// @param [in,out] state The propagation state object
  /// @param [in] navigator The navigator object
  /// @param logger a logger instance
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  bool checkAbort(propagator_state_t& state, const stepper_t& /*stepper*/,
                  const navigator_t& navigator, const Logger& logger) const {
    const auto& constrainToVolumeIds = state.options.constrainToVolumeIds;
    const auto& endOfWorldVolumeIds = state.options.endOfWorldVolumeIds;

    if (constrainToVolumeIds.empty() && endOfWorldVolumeIds.empty()) {
      return false;
    }
    const auto* currentVolume = navigator.currentVolume(state.navigation);

    // We need a volume to check its ID
    if (currentVolume == nullptr) {
      return false;
    }

    const auto currentVolumeId =
        static_cast<std::uint32_t>(currentVolume->geometryId().volume());

    if (!constrainToVolumeIds.empty() &&
        std::find(constrainToVolumeIds.begin(), constrainToVolumeIds.end(),
                  currentVolumeId) == constrainToVolumeIds.end()) {
      ACTS_VERBOSE(
          "VolumeConstraintAborter aborter | Abort with volume constrain "
          << currentVolumeId);
      return true;
    }

    if (!endOfWorldVolumeIds.empty() &&
        std::find(endOfWorldVolumeIds.begin(), endOfWorldVolumeIds.end(),
                  currentVolumeId) != endOfWorldVolumeIds.end()) {
      ACTS_VERBOSE(
          "VolumeConstraintAborter aborter | Abort with additional end of "
          "world volume "
          << currentVolumeId);
      return true;
    }

    return false;
  }
};

/// Aborter that checks if the propagation has reached any surface
struct AnySurfaceReached {
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  bool checkAbort(propagator_state_t& state, const stepper_t& stepper,
                  const navigator_t& navigator, const Logger& logger) const {
    (void)stepper;
    (void)logger;

    const Surface* startSurface = navigator.startSurface(state.navigation);
    const Surface* targetSurface = navigator.targetSurface(state.navigation);
    const Surface* currentSurface = navigator.currentSurface(state.navigation);

    // `startSurface` is excluded because we want to reach a new surface
    // `targetSurface` is excluded because another aborter should handle it
    if (currentSurface != nullptr && currentSurface != startSurface &&
        currentSurface != targetSurface) {
      return true;
    }

    return false;
  }
};

}  // namespace Acts
