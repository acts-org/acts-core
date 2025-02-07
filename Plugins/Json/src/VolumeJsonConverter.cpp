// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Json/VolumeJsonConverter.hpp"

#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Plugins/Json/GeometryJsonKeys.hpp"
#include "Acts/Plugins/Json/MaterialJsonConverter.hpp"

void Acts::to_json(
    nlohmann::json& j,
    const std::pair<const Acts::TrackingVolume*,
                    std::shared_ptr<const Acts::IVolumeMaterial>>& volume) {
  j[Acts::jsonKey().namekey] = volume.first->volumeName();
  to_json(j, volume.second.get());
}

void Acts::to_json(nlohmann::json& j, const Acts::TrackingVolume& volume) {
  j[Acts::jsonKey().namekey] = volume.volumeName();
  if (volume.volumeMaterial() != nullptr) {
    to_json(j, volume.volumeMaterial());
  }
  return;
}
