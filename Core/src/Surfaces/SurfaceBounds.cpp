// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <Acts/Surfaces/SurfaceBounds.hpp>

namespace Acts {

bool SurfaceBounds::inside(const Vector2& lposition,
                           const BoundaryTolerance& boundaryTolerance) const {
  using enum BoundaryTolerance::Mode;

  if (boundaryTolerance.isInfinite()) {
    return true;
  }

  BoundaryTolerance::Mode toleranceMode = boundaryTolerance.mode();
  bool strictlyInside = inside(lposition);

  if (toleranceMode == None) {
    return strictlyInside;
  }

  if (toleranceMode == Extend && strictlyInside) {
    return true;
  }

  std::optional<SquareMatrix2> jacobian;
  std::optional<SquareMatrix2> metric;
  if (boundaryTolerance.hasChi2Bound()) {
    SquareMatrix2 j = boundToCartesianJacobian(lposition);
    jacobian = j;
    metric = j.transpose() * boundaryTolerance.asChi2Bound().weight * j;
  } else if (!boundaryTolerance.hasAbsoluteBound(isCartesian())) {
    jacobian = boundToCartesianJacobian(lposition);
    metric = boundToCartesianMetric(lposition);
  }

  Vector2 closest = closestPoint(lposition, metric);
  Vector2 distance = closest - lposition;

  if (toleranceMode == Shrink) {
    return boundaryTolerance.isTolerated(distance, jacobian) && strictlyInside;
  }
  return boundaryTolerance.isTolerated(distance, jacobian);
}

}  // namespace Acts
