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
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"

#include <memory>
#include <utility>

namespace Acts {

class DiscBounds;

/// @class DiscLayer
///
/// Class to describe a disc-like detector layer for tracking,
/// it inhertis from both, Layer base class
/// and DiscSurface class

class DiscLayer : virtual public DiscSurface, public Layer {
 public:
  ///  Factory constructor with DiscSurface components
  ///
  /// @param transform is the transform to place the layer in the 3D frame
  /// @param dbounds are the disc bounds that describe the layer dimensions
  /// @param surfaceArray is the array of sensitive surfaces
  /// @param thickness is the layer thickness (along the normal vector)
  /// @param ad is the approach descriptor that provides the approach surface
  /// @param laytyp is the layer type
  ///
  /// @todo move ApproachDescriptor to unqique_ptr
  ///
  /// @return a sharted pointer to the new layer
  static std::shared_ptr<DiscLayer> create(
      const Transform3& transform,
      const std::shared_ptr<const DiscBounds>& dbounds,
      std::unique_ptr<SurfaceArray> surfaceArray = nullptr,
      double thickness = 0., std::unique_ptr<ApproachDescriptor> ad = nullptr,
      LayerType laytyp = Acts::passive) {
    return std::shared_ptr<DiscLayer>(
        new DiscLayer(transform, dbounds, std::move(surfaceArray), thickness,
                      std::move(ad), laytyp));
  }

  DiscLayer() = delete;
  DiscLayer(const DiscLayer& cla) = delete;
  ~DiscLayer() override = default;
  DiscLayer& operator=(const DiscLayer&) = delete;

  /// Transforms the layer into a Surface representation for extrapolation
  /// @return This method returns a surface reference
  const DiscSurface& surfaceRepresentation() const override;

  // Non-const version
  DiscSurface& surfaceRepresentation() override;

 private:
  /// build approach surfaces
  void buildApproachDescriptor();

 protected:
  // Constructor with DiscSurface components and pointer to SurfaceArray
  ///
  /// @param transform is the transform to place the layer in the 3D frame
  /// @param dbounds are the disc bounds that describe the layer dimensions
  /// @param surfaceArray is the array of sensitive surfaces
  /// @param thickness is the layer thickness (along the normal vector)
  /// @param ades Are the approach descriptors that provides the approach surface
  /// @param laytyp is the layer taype
  DiscLayer(const Transform3& transform,
            const std::shared_ptr<const DiscBounds>& dbounds,
            std::unique_ptr<SurfaceArray> surfaceArray = nullptr,
            double thickness = 0.,
            std::unique_ptr<ApproachDescriptor> ades = nullptr,
            LayerType laytyp = Acts::active);

  /// Copy constructor with shift
  DiscLayer(const DiscLayer& cla, const Transform3& tr);
};

}  // namespace Acts
