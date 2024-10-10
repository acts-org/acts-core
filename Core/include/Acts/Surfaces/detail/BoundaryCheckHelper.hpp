// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Surfaces/detail/VerticesHelper.hpp"

#include <span>

namespace Acts::detail {

/// Check if a point is inside a box.
///
/// @param lowerLeft The lower left corner of the box.
/// @param upperRight The upper right corner of the box.
/// @param tolerance The tolerance to use.
/// @param point The point to check.
/// @param jacobianOpt The Jacobian to transform the distance to Cartesian
///
/// @return True if the point is inside the box.
inline bool insideAlignedBox(const Vector2& lowerLeft,
                             const Vector2& upperRight,
                             const BoundaryTolerance& tolerance,
                             const Vector2& point,
                             const std::optional<SquareMatrix2>& jacobianOpt) {
  if (tolerance.isInfinite()) {
    return true;
  }

  if (detail::VerticesHelper::isInsideRectangle(point, lowerLeft, upperRight)) {
    return true;
  }

  if (!tolerance.hasTolerance()) {
    return false;
  }

  Vector2 closestPoint;

  if (!tolerance.hasMetric(jacobianOpt.has_value())) {
    closestPoint =
        detail::VerticesHelper::computeEuclideanClosestPointOnRectangle(
            point, lowerLeft, upperRight);
  } else {
    // TODO there might be a more optimal way to compute the closest point to a
    // box with metric

    std::array<Vector2, 4> vertices = {{lowerLeft,
                                        {upperRight[0], lowerLeft[1]},
                                        upperRight,
                                        {lowerLeft[0], upperRight[1]}}};

    SquareMatrix2 metric = tolerance.getMetric(jacobianOpt);

    closestPoint = detail::VerticesHelper::computeClosestPointOnPolygon(
        point, vertices, metric);
  }

  Vector2 distance = closestPoint - point;

  return tolerance.isTolerated(distance, jacobianOpt);
}

/// Check if a point is inside a polygon.
///
/// @param vertices The vertices of the polygon.
/// @param tolerance The tolerance to use.
/// @param point The point to check.
/// @param jacobianOpt The Jacobian to transform the distance to Cartesian
///
/// @return True if the point is inside the polygon.
inline bool insidePolygon(std::span<const Vector2> vertices,
                          const BoundaryTolerance& tolerance,
                          const Vector2& point,
                          const std::optional<SquareMatrix2>& jacobianOpt) {
  if (tolerance.isInfinite()) {
    // The null boundary check always succeeds
    return true;
  }

  if (detail::VerticesHelper::isInsidePolygon(point, vertices)) {
    // If the point falls inside the polygon, the check always succeeds
    return true;
  }

  if (!tolerance.hasTolerance()) {
    // Outside of the polygon, since we've eliminated the case of an absence of
    // check above, we know we'll always fail if the tolerance is zero.
    //
    // This allows us to avoid the expensive computeClosestPointOnPolygon
    // computation in this simple case.
    return false;
  }

  // TODO: When tolerance is not 0, we could also avoid this computation in
  //       some cases by testing against a bounding box of the polygon, padded
  //       on each side with our tolerance. Check if this optimization is
  //       worthwhile in some production workflows, and if so implement it.

  SquareMatrix2 metric = tolerance.getMetric(jacobianOpt);

  // We are outside of the polygon, but there is a tolerance. Must find what
  // the closest point on the polygon is and check if it's within tolerance.
  auto closestPoint = detail::VerticesHelper::computeClosestPointOnPolygon(
      point, vertices, metric);

  Vector2 distance = closestPoint - point;

  return tolerance.isTolerated(distance, jacobianOpt);
}

}  // namespace Acts::detail
