// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/BinnedArray.hpp"

#include <memory>
#include <vector>

namespace Acts {

class TrackingVolume;

/// A std::shared_ptr to a tracking volume
using TrackingVolumePtr = std::shared_ptr<const TrackingVolume>;
using MutableTrackingVolumePtr = std::shared_ptr<TrackingVolume>;

/// A BinnedArray of a std::shared_tr to a TrackingVolume
using TrackingVolumeArray = BinnedArray<TrackingVolumePtr>;
/// A std::vector of a std::shared_ptr to a TrackingVolume
using TrackingVolumeVector = std::vector<TrackingVolumePtr>;

/// @class ITrackingVolumeArrayCreator
///
/// Interface class ITrackingVolumeArrayCreators It inherits from IAlgTool.
///
/// It is designed to centralize the code to create
/// Arrays of Tracking Volumes for both:
///
///   - confinement in another TrackingVolume
///   - navigation and glueing
///
/// Arrays for glueing and confinement are often the same,
/// therefore the newly created TrackingVolumeArray is done by a shared_ptr
class ITrackingVolumeArrayCreator {
 public:
  /// Virtual destructor
  virtual ~ITrackingVolumeArrayCreator() = default;

  /// TrackingVolumeArrayCreator interface method - creates array depending on
  /// the binning type
  ///
  /// @param [in] gctx the geometry context for this building
  /// @param vols are the TrackingVolumes ordered in a tracker
  /// @param aDir is the axis direction for the volume binning
  ///
  /// @return shared pointer to a new TrackingVolumeArray
  virtual std::shared_ptr<const TrackingVolumeArray> trackingVolumeArray(
      const GeometryContext& gctx, const TrackingVolumeVector& vols,
      AxisDirection aDir) const = 0;
};
}  // namespace Acts
