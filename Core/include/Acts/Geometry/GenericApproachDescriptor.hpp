// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/ApproachDescriptor.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Intersection.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace Acts {

class Layer;
class Surface;

/// @class GenericApproachDescriptor
///
/// Class to decide and return which approaching surface to be taken,
/// it's a generic descriptor for n surfaces
class GenericApproachDescriptor : public ApproachDescriptor {
 public:
  /// A generic approach descriptor for new Acts::Surface objects
  /// passing ownership
  ///
  /// @param aSurfaces are the approach surfaces
  GenericApproachDescriptor(
      std::vector<std::shared_ptr<const Surface>> aSurfaces)
      : ApproachDescriptor(),
        m_surfaces(std::move(aSurfaces)),
        m_surfaceCache() {
    m_surfaceCache = unpack_shared_vector(m_surfaces);
  }

  /// A generic approach descriptor with n surfaces to test
  ~GenericApproachDescriptor() override = default;

  /// @brief Register the Layer to the surfaces
  ///
  /// @param lay is the layer to be registered
  void registerLayer(const Layer& lay) override;

  /// Get the approach surface to the layer
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position The global position to start the approach from
  /// @param direction The momentum vector
  /// @param boundaryTolerance The boundary check prescription
  /// @param nearLimit The minimum distance for an intersection to be considered
  /// @param farLimit The maximum distance for an intersection to be considered
  ///
  /// @return : a @c SurfaceIntersection
  SurfaceIntersection approachSurface(
      const GeometryContext& gctx, const Vector3& position,
      const Vector3& direction, const BoundaryTolerance& boundaryTolerance,
      double nearLimit, double farLimit) const override;

  /// return all contained surfaces of this approach descriptor
  const std::vector<const Surface*>& containedSurfaces() const override;

  /// Non-const version
  std::vector<const Surface*>& containedSurfaces() override;

 private:
  /// approach surfaces with ownership control
  std::vector<std::shared_ptr<const Surface>> m_surfaces;

  /// the surface container cache
  ///
  /// We will need to mutate those surfaces in registerLayer, but the C++ type
  /// system has no const-correct way of expressing this constraint.
  ///
  std::vector<const Surface*> m_surfaceCache;
};

}  // namespace Acts
