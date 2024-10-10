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

/// @brief Helpers for planar surfaces that share the same maths
namespace Acts::PlanarHelper {

/// Intersection with a planar surface
///
/// @param transform The 3D affine transform that places the surface
/// @param position The starting position for the intersection
/// @param direction The starting direction for the intersection
///
/// @return The intersection
inline Intersection3D intersect(const Transform3& transform,
                                const Vector3& position,
                                const Vector3& direction,
                                ActsScalar tolerance) {
  // Get the matrix from the transform (faster access)
  const auto& tMatrix = transform.matrix();
  const Vector3 pnormal = tMatrix.block<3, 1>(0, 2).transpose();
  const Vector3 pcenter = tMatrix.block<3, 1>(0, 3).transpose();
  // It is solvable, so go on
  ActsScalar denom = direction.dot(pnormal);
  if (denom != 0.0) {
    // Translate that into a path
    ActsScalar path = (pnormal.dot((pcenter - position))) / (denom);
    // Is valid hence either on surface or reachable
    Intersection3D::Status status = std::abs(path) < std::abs(tolerance)
                                        ? Intersection3D::Status::onSurface
                                        : Intersection3D::Status::reachable;
    // Return the intersection
    return Intersection3D{(position + path * direction), path, status};
  }
  return Intersection3D::invalid();
}

}  // namespace Acts::PlanarHelper
