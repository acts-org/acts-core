// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

namespace Acts {

class TrackingVolume;
class Portal;
class Surface;
class Layer;

template <typename T>
class BoundarySurfaceT;

/// @brief Visitor interface for traversing the tracking geometry hierarchy
///
/// This visitor allows for const access to traverse and inspect the tracking
/// geometry components without modifying them. It's used for operations like
/// visualization, validation, or collecting information about the geometry
/// structure.
class TrackingGeometryVisitor {
 public:
  virtual ~TrackingGeometryVisitor() = 0;

  /// @brief Visit a tracking volume in the geometry
  /// @param volume The tracking volume being visited
  /// @note Called for each volume in the geometry hierarchy during traversal
  virtual void visitVolume(const TrackingVolume& volume);

  /// @brief Visit a portal (boundary between volumes)
  /// @param portal The portal being visited
  /// @note Called for each portal encountered during geometry traversal
  virtual void visitPortal(const Portal& portal);

  /// @brief Visit a surface in the geometry
  /// @param surface The surface being visited
  /// @note Called for each surface encountered during geometry traversal
  virtual void visitSurface(const Surface& surface);

  // Gen 1
  /// @brief Visit a detector layer
  /// @param layer The layer being visited
  /// @note Called for each layer encountered during geometry traversal
  virtual void visitLayer(const Layer& layer);

  /// @brief Visit a boundary surface between tracking volumes
  /// @param boundary The boundary surface being visited
  /// @note Called for each boundary surface encountered during geometry traversal
  virtual void visitBoundarySurface(
      const BoundarySurfaceT<TrackingVolume>& boundary);
};

/// @brief Mutable visitor interface for modifying the tracking geometry hierarchy
///
/// This visitor allows for non-const access to traverse and modify the tracking
/// geometry components. It's used for operations like geometry construction,
/// material decoration, or geometry ID assignment.
class TrackingGeometryMutableVisitor {
 public:
  virtual ~TrackingGeometryMutableVisitor();

  /// @brief Visit and potentially modify a tracking volume
  /// @param volume The tracking volume being visited
  /// @note Called for each volume in the geometry hierarchy during traversal
  virtual void visitVolume(TrackingVolume& volume);

  /// @brief Visit and potentially modify a portal
  /// @param portal The portal being visited
  /// @note Called for each portal encountered during geometry traversal
  virtual void visitPortal(Portal& portal);

  /// @brief Visit and potentially modify a surface
  /// @param surface The surface being visited
  /// @note Called for each surface encountered during geometry traversal
  virtual void visitSurface(Surface& surface);

  // Gen 1
  /// @brief Visit and potentially modify a detector layer
  /// @param layer The layer being visited
  /// @note Called for each layer encountered during geometry traversal
  virtual void visitLayer(Layer& layer);

  /// @brief Visit and potentially modify a boundary surface
  /// @param boundary The boundary surface being visited
  /// @note Called for each boundary surface encountered during geometry traversal
  virtual void visitBoundarySurface(BoundarySurfaceT<TrackingVolume>& boundary);
};

}  // namespace Acts
