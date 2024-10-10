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
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"

#include <memory>
#include <utility>

namespace Acts {

class PlanarBounds;

/// @class PlaneLayer
///
/// Class to describe a planar detector layer for tracking,
/// it inhertis from both, Layer base class and PlaneSurface class
///
class PlaneLayer : virtual public PlaneSurface, public Layer {
 public:
  /// Factory for a shared plane layer
  ///
  /// @param transform which places the layer in the global frame
  /// @param pbounds the planar bounds that define the layer dimensions
  /// @param surfaceArray is the surface array that holds the sensitive surfaces
  /// @param thickness is the thickness of the layer (normal direction to plane)
  /// @param ad is the approach descriptor for describing the approach surface
  /// @param laytyp is the layer type
  ///
  /// @return shared pointer to a PlaneLayer
  static MutableLayerPtr create(
      const Transform3& transform, std::shared_ptr<const PlanarBounds> pbounds,
      std::unique_ptr<SurfaceArray> surfaceArray = nullptr,
      double thickness = 0., std::unique_ptr<ApproachDescriptor> ad = nullptr,
      LayerType laytyp = Acts::active) {
    return MutableLayerPtr(new PlaneLayer(transform, pbounds,
                                          std::move(surfaceArray), thickness,
                                          std::move(ad), laytyp));
  }

  PlaneLayer() = delete;
  PlaneLayer(const PlaneLayer& pla) = delete;
  ~PlaneLayer() override = default;
  PlaneLayer& operator=(const PlaneLayer&) = delete;

  /// Transforms the layer into a Surface representation for extrapolation
  /// @return returns a reference to a PlaneSurface
  const PlaneSurface& surfaceRepresentation() const override;

  // Non-const version
  PlaneSurface& surfaceRepresentation() override;

 private:
  /// private helper method to build approach surfaces
  void buildApproachDescriptor();

 protected:
  /// Private constructor for a PlaneLayer is called by create(args*)
  ///
  /// @param transform which places the layer in the global frame
  /// @param pbounds the planar bounds that define the layer dimensions
  /// @param surfaceArray is the surface array that holds the sensitive surfaces
  /// @param thickness is the thickness of the layer (normal direction to plane)
  /// @param ades is the approach descriptor for describing the approach surface
  /// @param laytyp is the layer type
  PlaneLayer(const Transform3& transform,
             std::shared_ptr<const PlanarBounds>& pbounds,
             std::unique_ptr<SurfaceArray> surfaceArray = nullptr,
             double thickness = 0.,
             std::unique_ptr<ApproachDescriptor> ades = nullptr,
             LayerType laytyp = Acts::active);

  /// Private constructor for a PlaneLayer, is called by create(arge*
  ///
  /// @param pla is the plain layer to be coped
  /// @param shift is the additional shift applied after copying
  PlaneLayer(const PlaneLayer& pla, const Transform3& shift);
};

}  // namespace Acts
