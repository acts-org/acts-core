// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Intersection.hpp"

#include <vector>

namespace Acts {

class Surface;
class Layer;

/// @class ApproachDescriptor
///
/// Virtual base class to decide and return which approaching surface to be
/// taken, the surfaces are std::shared_ptr, as they can be the boundary
/// surfaces of the representingVolume of the Layer
class ApproachDescriptor {
 public:
  ApproachDescriptor() = default;
  virtual ~ApproachDescriptor() = default;

  /// @brief Register Layer
  /// Links the layer to the approach surfaces
  ///
  /// @param lay is the layer to be assigned
  virtual void registerLayer(const Layer& lay) = 0;

  /// @brief Get the surface on approach
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position is the position from start of the search
  /// @param direction is the direction at the start of the search
  /// @param boundaryTolerance is the boundary check directive
  /// @param nearLimit The minimum distance for an intersection to be considered
  /// @param farLimit The maximum distance for an intersection to be considered
  ///
  /// @return is a surface intersection
  virtual SurfaceIntersection approachSurface(
      const GeometryContext& gctx, const Vector3& position,
      const Vector3& direction, const BoundaryTolerance& boundaryTolerance,
      double nearLimit, double farLimit) const = 0;

  /// Get all the contained surfaces
  /// @return all contained surfaces of this approach descriptor
  virtual const std::vector<const Surface*>& containedSurfaces() const = 0;

  /// Non-const version
  virtual std::vector<const Surface*>& containedSurfaces() = 0;
};

}  // namespace Acts
