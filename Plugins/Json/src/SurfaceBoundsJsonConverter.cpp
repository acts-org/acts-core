// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Json/SurfaceBoundsJsonConverter.hpp"

#include "Acts/Plugins/Json/DetrayJsonHelper.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

void Acts::to_json(nlohmann::json& j, const Acts::SurfaceBounds& bounds) {
  j["type"] = bounds.type();
  j["values"] = bounds.values();
}

nlohmann::json Acts::SurfaceBoundsJsonConverter::toJson(
    const Acts::SurfaceBounds& bounds) {
  return nlohmann::json(bounds);
}

nlohmann::json Acts::SurfaceBoundsJsonConverter::toJsonDetray(
    const Acts::SurfaceBounds& bounds, bool portal) {
  nlohmann::json jMask;
  auto [shape, boundaries] = DetrayJsonHelper::maskFromBounds(bounds, portal);
  jMask["shape"] = shape;
  jMask["boundaries"] = boundaries;
  return jMask;
}
