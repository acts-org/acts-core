// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Propagator/StandardAborters.hpp"
#include "Acts/Surfaces/Surface.hpp"

namespace Acts {

struct MultiStepperSurfaceReached : public SurfaceReached {
  /// If this is set, we are also happy if the mean of the components is on the
  /// surface. How the averaging is performed depends on the stepper
  /// implementation
  bool averageOnSurface = true;

  /// A configurable tolerance within which distance to the intersection we
  /// consider the surface as reached. Has no effect if averageOnSurface is
  /// false
  double averageOnSurfaceTolerance = 0.2;

  MultiStepperSurfaceReached() = default;
  explicit MultiStepperSurfaceReached(double oLimit) : SurfaceReached(oLimit) {}

  /// boolean operator for abort condition without using the result
  ///
  /// @tparam propagator_state_t Type of the propagator state
  /// @tparam stepper_t Type of the stepper
  /// @tparam navigator_t Type of the navigator
  ///
  /// @param [in,out] state The propagation state object
  /// @param [in] stepper Stepper used for propagation
  /// @param [in] navigator Navigator used for the propagation
  /// @param logger a logger instance
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  bool checkAbort(propagator_state_t& state, const stepper_t& stepper,
                  const navigator_t& navigator, const Logger& logger) const {
    if (surface == nullptr) {
      ACTS_VERBOSE(
          "MultiStepperSurfaceReached aborter | "
          "No target surface set.");
      return false;
    }

    // However, if mean of all is on surface, we are happy as well
    if (averageOnSurface) {
      const auto sIntersection =
          surface
              ->intersect(
                  state.geoContext, stepper.position(state.stepping),
                  state.options.direction * stepper.direction(state.stepping),
                  BoundaryTolerance(boundaryTolerance),
                  averageOnSurfaceTolerance)
              .closest();

      if (sIntersection.status() == Intersection3D::Status::onSurface) {
        ACTS_VERBOSE(
            "MultiStepperSurfaceReached aborter | "
            "Reached target in average mode");
        for (auto cmp : stepper.componentIterable(state.stepping)) {
          cmp.status() = Intersection3D::Status::onSurface;
        }

        return true;
      }

      ACTS_VERBOSE(
          "MultiStepperSurfaceReached aborter | "
          "Target intersection not found. Maybe next time?");
    }

    bool reached = true;

    for (auto cmp : stepper.componentIterable(state.stepping)) {
      // note that this is not copying anything heavy
      auto singleState = cmp.singleState(state);
      const auto& singleStepper = cmp.singleStepper(stepper);

      if (!SurfaceReached::checkAbort(singleState, singleStepper, navigator,
                                      logger)) {
        reached = false;
      } else {
        cmp.status() = Acts::Intersection3D::Status::onSurface;
      }
    }

    if (reached) {
      ACTS_VERBOSE(
          "MultiStepperSurfaceReached aborter | "
          "Reached target in single component mode");
    }

    return reached;
  }
};

}  // namespace Acts
