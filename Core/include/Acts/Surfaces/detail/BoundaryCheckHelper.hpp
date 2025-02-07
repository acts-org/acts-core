// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Surfaces/BoundaryTolerance.hpp"
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
  using enum BoundaryTolerance::ToleranceMode;

  if (tolerance.isInfinite()) {
    return true;
  }

  BoundaryTolerance::ToleranceMode mode = tolerance.toleranceMode();
  bool insideRectangle =
      detail::VerticesHelper::isInsideRectangle(point, lowerLeft, upperRight);

  if (mode == None) {
    return insideRectangle;
  }

  if (mode == Extend && insideRectangle) {
    return true;
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

  if (mode == Extend) {
    return tolerance.isTolerated(distance, jacobianOpt);
  } else {
    return tolerance.isTolerated(distance, jacobianOpt) && insideRectangle;
  }
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
  using enum BoundaryTolerance::ToleranceMode;
  if (tolerance.isInfinite()) {
    // The null boundary check always succeeds
    return true;
  }

  BoundaryTolerance::ToleranceMode mode = tolerance.toleranceMode();
  bool insidePolygon = detail::VerticesHelper::isInsidePolygon(point, vertices);

  if (mode == None) {
    // If the point falls inside the polygon, the check always succeeds
    // Outside of the polygon, since we've eliminated the case of an absence of
    // check above, we know we'll always fail if the tolerance is zero.
    //
    // This allows us to avoid the expensive computeClosestPointOnPolygon
    // computation in this simple case.
    return insidePolygon;
  }

  if (mode == Extend && insidePolygon) {
    return true;
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

  if (mode == Extend) {
    return tolerance.isTolerated(distance, jacobianOpt);
  } else {
    // @TODO: Check sign
    return tolerance.isTolerated(-distance, jacobianOpt) && insidePolygon;
  }
}

}  // namespace Acts::detail
