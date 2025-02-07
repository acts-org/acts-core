// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Json/ActsJson.hpp"

namespace Acts {
class TrackingVolume;
class Surface;

/// helper class to add extra information to surface or volume json objects
class ITrackingGeometryJsonDecorator {
 public:
  virtual ~ITrackingGeometryJsonDecorator() = default;

  /// Add extra elements to the json object already filled for the given
  /// surface
  ///
  /// @param surface the surface which was used to fill the json object
  /// @param json the json object that is enhanced
  virtual void decorate(const Acts::Surface &surface,
                        nlohmann::json &json) const = 0;

  /// Add extra elements to the json object already filled for the given
  /// volume
  ///
  /// @param volume the volume which was used to fill the json object
  /// @param json the json object that is enhanced
  virtual void decorate(const Acts::TrackingVolume &volume,
                        nlohmann::json &json) const = 0;
};
}  // namespace Acts
