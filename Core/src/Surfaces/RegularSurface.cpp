// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/RegularSurface.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

namespace Acts {

Vector3 RegularSurface::normal(const GeometryContext& gctx, const Vector3& pos,
                               const Vector3& /*direction*/) const {
  return normal(gctx, pos);
}

Result<Vector2> RegularSurface::globalToLocal(const GeometryContext& gctx,
                                              const Vector3& position,
                                              const Vector3& /*direction*/,
                                              double tolerance) const {
  return globalToLocal(gctx, position, tolerance);
}

Vector3 RegularSurface::localToGlobal(const GeometryContext& gctx,
                                      const Vector2& lposition,
                                      const Vector3& /*direction*/) const {
  return localToGlobal(gctx, lposition);
}

bool RegularSurface::isOnSurface(const GeometryContext& gctx,
                                 const Vector3& position,
                                 const BoundaryTolerance& boundaryTolerance,
                                 double tolerance) const {
  // global to local transformation
  auto lpResult = globalToLocal(gctx, position, tolerance);
  if (!lpResult.ok()) {
    return false;
  }
  return bounds().inside(lpResult.value(), boundaryTolerance);
}

}  // namespace Acts
