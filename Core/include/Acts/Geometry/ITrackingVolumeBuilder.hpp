// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
