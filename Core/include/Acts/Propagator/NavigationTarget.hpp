// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
