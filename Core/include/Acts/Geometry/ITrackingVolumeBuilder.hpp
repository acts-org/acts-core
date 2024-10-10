// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"

#include <memory>
#include <tuple>
#include <vector>

namespace Acts {

class VolumeBounds;
class TrackingVolume;
class Layer;
class Volume;
using TrackingVolumePtr = std::shared_ptr<const TrackingVolume>;
using MutableTrackingVolumePtr = std::shared_ptr<TrackingVolume>;
using LayerPtr = std::shared_ptr<const Layer>;
using LayerVector = std::vector<LayerPtr>;
///  @class ITrackingVolumeBuilder
///
/// Interface class ITrackingVolumeBuilders
///
/// this returns the sub-detector tracking volume that is wrapped by the next
/// outer one
/// in the TrackingGeometry building process
///
/// If an innerVolume is given, this is wrapped
/// If a VolumeBounds object is given this defines the maximum extent.
///

class ITrackingVolumeBuilder {
 public:
  /// Virtual destructor
  virtual ~ITrackingVolumeBuilder() = default;

  /// ITrackingVolumeBuilder interface method
  ///
  /// @param gctx is the geometry context for witch the volume is built
  /// @param oppositeVolume is an (optional) volume to be wrapped
  /// @param outsideBounds is an (optional) outside confinement
  ///
  /// @return shared pointer to a newly created TrackingVolume
  virtual MutableTrackingVolumePtr trackingVolume(
      const GeometryContext& gctx, TrackingVolumePtr oppositeVolume = nullptr,
      std::shared_ptr<const VolumeBounds> outsideBounds = nullptr) const = 0;
};

}  // namespace Acts
