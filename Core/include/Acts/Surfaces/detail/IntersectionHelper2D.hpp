// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/Intersection.hpp"

#include <array>

namespace Acts::detail {

struct IntersectionHelper2D {
  /// Intersect two segments
  ///
  /// @param s0 The Start of the segment
  /// @param s1 The end of the segment
  /// @param origin The Start of intersection line
  /// @param dir The Direction of intersection line
  ///
  /// @return the intersection point with status
  static Intersection2D intersectSegment(const Vector2& s0, const Vector2& s1,
                                         const Vector2& origin,
                                         const Vector2& dir,
                                         bool boundCheck = false);

  /// Intersect ellipses
  ///
  /// @param Rx The radius in x
  /// @param Ry The radius in y
  /// @param origin The Start of intersection line
  /// @param dir The Direction of intersection line
  ///
  /// @return the intersection points
  static std::array<Intersection2D, 2> intersectEllipse(ActsScalar Rx,
                                                        ActsScalar Ry,
                                                        const Vector2& origin,
                                                        const Vector2& dir);

  /// Intersect the circle
  ///
  /// @param R The radius
  /// @param origin The Start of intersection line
  /// @param dir The Direction of intersection line
  ///
  /// @return the intersection points
  static inline std::array<Intersection2D, 2> intersectCircle(
      ActsScalar R, const Vector2& origin, const Vector2& dir) {
    return intersectEllipse(R, R, origin, dir);
  }

  /// Intersect a circle segment
  ///
  /// @note only forward solution is taken
  ///
  /// @param R The radius
  /// @param phiMin The minimum phi value
  /// @param phiMax The maximum phi value
  /// @param origin The Start of intersection line
  /// @param dir The Direction of intersection line
  ///
  /// @return the intersection points
  static Intersection2D intersectCircleSegment(ActsScalar R, ActsScalar phiMin,
                                               ActsScalar phiMax,
                                               const Vector2& origin,
                                               const Vector2& dir);

};  // struct IntersectionHelper2D

}  // namespace Acts::detail
