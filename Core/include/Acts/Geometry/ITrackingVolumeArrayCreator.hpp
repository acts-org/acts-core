// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/BinnedArray.hpp"
#include "Acts/Utilities/BinningType.hpp"

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
  /// @param bVal is the binning value for the volume binning
  ///
  /// @return shared pointer to a new TrackingVolumeArray
  virtual std::shared_ptr<const TrackingVolumeArray> trackingVolumeArray(
      const GeometryContext& gctx, const TrackingVolumeVector& vols,
      BinningValue bVal) const = 0;
};
}  // namespace Acts
