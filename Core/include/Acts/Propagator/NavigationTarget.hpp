// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Surfaces/BoundaryTolerance.hpp"

#include <utility>

namespace Acts {

class Surface;

/// @brief The navigation target
///
/// This struct represents a navigation target which is communicated from the
/// navigator to the stepper through the propagator.
///
/// @note This incorporates `std::optional` semantics as the next target might
///       not exist.
struct NavigationTarget {
  const Surface* surface = nullptr;
  std::uint8_t surfaceIntersectionIndex = 0;
  BoundaryTolerance boundaryTolerance = BoundaryTolerance::None();

  static NavigationTarget None() { return NavigationTarget(); }

  NavigationTarget(const Surface& surface_,
                   std::uint8_t surfaceIntersectionIndex_,
                   BoundaryTolerance boundaryTolerance_)
      : surface(&surface_),
        surfaceIntersectionIndex(surfaceIntersectionIndex_),
        boundaryTolerance(std::move(boundaryTolerance_)) {}

  bool isNone() const { return surface == nullptr; }

 private:
  NavigationTarget() = default;
};

}  // namespace Acts
