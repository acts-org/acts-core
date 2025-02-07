// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>
#include <vector>

namespace Acts {

class TrackingVolume;
using MutableTrackingVolumePtr = std::shared_ptr<TrackingVolume>;
using MutableTrackingVolumeVector = std::vector<MutableTrackingVolumePtr>;

/// @brief This is an interface class for constructing TrackingVolumes whose are
/// confined in a mother-TrackingVolume
class IConfinedTrackingVolumeBuilder {
 public:
  /// Virtual destructor
  virtual ~IConfinedTrackingVolumeBuilder() = default;

  /// Interface for constructing a vector of confined TrackingVolumes
  virtual MutableTrackingVolumeVector centralVolumes() const = 0;

  /// Interface for retrieving the identification string of the confined volumes
  virtual const std::string& identification() const = 0;
};

}  // namespace Acts
