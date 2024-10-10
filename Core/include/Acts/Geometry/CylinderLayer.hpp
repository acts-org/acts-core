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
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace Acts {
class CylinderBounds;

/// @class CylinderLayer
///
/// Class to describe a cylindrical detector layer for tracking, it inhertis
/// from both, Layer base class and CylinderSurface class
///
class CylinderLayer : public CylinderSurface, public Layer {
 public:
  /// Factory for shared Layer pointer
  /// create a shared, fully deployed CylinderLayer
  ///
  /// @param transform is the 3D transform that places the layer in 3D space
  /// @param cbounds are the cylindrical bounds of the layer
  /// @param surfaceArray is the Binned Array that holds the sensitive surfaces
  /// @param thickness is the layer thickness (along the normal)
  /// @param ad is the approach descriptor for approaching the layer
  /// @param laytyp is the layer type
  ///
  /// @todo ApproachDescriptor to unique_ptr
  ///
  /// @return The return object is a shared pointer to the layer.
  static std::shared_ptr<CylinderLayer> create(
      const Transform3& transform,
      const std::shared_ptr<const CylinderBounds>& cbounds,
      std::unique_ptr<SurfaceArray> surfaceArray = nullptr,
      double thickness = 0., std::unique_ptr<ApproachDescriptor> ad = nullptr,
      LayerType laytyp = passive) {
    return std::shared_ptr<CylinderLayer>(
        new CylinderLayer(transform, cbounds, std::move(surfaceArray),
                          thickness, std::move(ad), laytyp));
  }

  CylinderLayer(const CylinderLayer& cla) = delete;
  CylinderLayer() = delete;
  ~CylinderLayer() override = default;
  CylinderLayer& operator=(const CylinderLayer&) = delete;

  /// Transforms the layer into a Surface representation
  /// This is for positioning and extrapolation
  const CylinderSurface& surfaceRepresentation() const override;

  // Non-const version
  CylinderSurface& surfaceRepresentation() override;

 private:
  /// build approach surfaces */
  void buildApproachDescriptor();

 protected:
  /// Private constructor for CylinderLayer, called by create(args*) factory
  ///
  /// @param transform is the 3D transform that places the layer in 3D space
  /// @param cBounds The cylindrical bounds of the layer
  /// @param surfaceArray is the Binned Array that holds the sensitive surfaces
  /// @param thickness is the layer thickness (along the normal)
  /// @param ades are the approach descriptors for approaching the layer
  /// @param laytyp is the layer type
  /// @todo change ApproachDescriptor to unique_ptr
  CylinderLayer(const Transform3& transform,
                const std::shared_ptr<const CylinderBounds>& cBounds,
                std::unique_ptr<SurfaceArray> surfaceArray = nullptr,
                double thickness = 0.,
                std::unique_ptr<ApproachDescriptor> ades = nullptr,
                LayerType laytyp = passive);

  /// Private copy constructor with shift, called by create(args*)
  ///
  /// @param cla is the source cylinder layer for the copy
  /// @param shift is the additional transform applied after cloning
  CylinderLayer(const CylinderLayer& cla, const Transform3& shift);
};

}  // namespace Acts
