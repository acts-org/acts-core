// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

namespace Acts {

/// A physical surface which does not depend on the direction you look at it
/// from. As such it narrows the interface of @c Surface and allows
/// inspection without providing a global position and direction.
class RegularSurface : public Surface {
 public:
  // Reuse all constructors from the base class
  using Surface::Surface;

  /// Calculate the normal vector of the surface
  /// This overload requires an on-surface local position
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param lposition is the local position where the normal vector is
  /// constructed
  ///
  /// @return normal vector by value
  virtual Vector3 normal(const GeometryContext& gctx,
                         const Vector2& lposition) const = 0;

  /// Calculate the normal vector of the surface
  /// This overload accepts a global position
  ///
  /// @param position is the global position where the normal vector is
  /// constructed
  /// @param gctx The current geometry context object, e.g. alignment
  /// @return normal vector by value
  virtual Vector3 normal(const GeometryContext& gctx,
                         const Vector3& position) const = 0;

  /// Calculate the normal vector of the surface
  /// This overload is fully generic, fulfills the @ref Surface interface and
  /// accepts a global position and a direction. For @c RegularSurface this is
  /// equivalent to the @ref normal
  /// overload, ignoring the @p direction
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param pos is the global position where the normal vector is constructed
  /// @param direction is the direction of the normal vector (ignored for @c RegularSurface)
  Vector3 normal(const GeometryContext& gctx, const Vector3& pos,
                 const Vector3& direction) const final;

  /// Convert a global position to a local one this is the most generic
  /// interface, which is implemented by all surfaces
  /// @note The @p position is required to be on-surface, which is indicated by
  ///       the `Result` return value.
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position is the global position to be converted
  /// @param direction is the direction of the local position (ignored for @c RegularSurface)
  /// @param tolerance is the tolerance for the on-surface check
  /// @return Result type containing local position by value
  Result<Vector2> globalToLocal(
      const GeometryContext& gctx, const Vector3& position,
      const Vector3& direction,
      double tolerance = s_onSurfaceTolerance) const final;

  /// Convert a global position to a local one.
  /// @note The @p position is required to be on-surface, which is indicated by
  ///       the `Result` return value.
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position is the global position to be converted
  /// @param tolerance is the tolerance for the on-surface check
  /// @return Result type containing local position by value
  virtual Result<Vector2> globalToLocal(
      const GeometryContext& gctx, const Vector3& position,
      double tolerance = s_onSurfaceTolerance) const = 0;

  /// Local to global transformation. This is the most generic interface,
  /// which is implemented by all surfaces.
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param lposition local 2D position in specialized surface frame
  /// @param direction global 3D momentum direction (ignored for @c RegularSurface)
  ///
  /// @return The global position by value
  Vector3 localToGlobal(const GeometryContext& gctx, const Vector2& lposition,
                        const Vector3& direction) const final;

  /// Local to global transformation.
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param lposition local 2D position in specialized surface frame
  ///
  /// @return The global position by value
  virtual Vector3 localToGlobal(const GeometryContext& gctx,
                                const Vector2& lposition) const = 0;

  /// The geometric onSurface method
  ///
  /// Geometrical check whether position is on Surface
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position global position to be evaludated
  /// @param boundaryTolerance BoundaryTolerance directive for this onSurface check
  /// @param tolerance optional tolerance within which a point is considered on surface
  ///
  /// @return boolean indication if operation was successful
  bool isOnSurface(
      const GeometryContext& gctx, const Vector3& position,
      const BoundaryTolerance& boundaryTolerance = BoundaryTolerance::None(),
      double tolerance = s_onSurfaceTolerance) const;

  using Surface::isOnSurface;
};
}  // namespace Acts
