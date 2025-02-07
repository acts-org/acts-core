// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Json/VolumeBoundsJsonConverter.hpp"

#include "Acts/Geometry/ConeVolumeBounds.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GenericCuboidVolumeBounds.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"

#include <algorithm>
#include <stdexcept>

void Acts::to_json(nlohmann::json& j, const VolumeBounds& bounds) {
  j["type"] = bounds.type();
  j["values"] = bounds.values();
}

nlohmann::json Acts::VolumeBoundsJsonConverter::toJson(
    const VolumeBounds& bounds) {
  return nlohmann::json(bounds);
}

std::unique_ptr<Acts::VolumeBounds> Acts::VolumeBoundsJsonConverter::fromJson(
    const nlohmann::json& jVolumeBounds) {
  const auto type = jVolumeBounds["type"].get<VolumeBounds::BoundsType>();

  switch (type) {
    case VolumeBounds::BoundsType::eCone:
      return fromJson<ConeVolumeBounds>(jVolumeBounds);
    case VolumeBounds::BoundsType::eCuboid:
      return fromJson<CuboidVolumeBounds>(jVolumeBounds);
    case VolumeBounds::BoundsType::eCutoutCylinder:
      return fromJson<CutoutCylinderVolumeBounds>(jVolumeBounds);
    case VolumeBounds::BoundsType::eCylinder:
      return fromJson<CylinderVolumeBounds>(jVolumeBounds);
    case VolumeBounds::BoundsType::eTrapezoid:
      return fromJson<TrapezoidVolumeBounds>(jVolumeBounds);
    case VolumeBounds::BoundsType::eGenericCuboid:
      return fromJson<GenericCuboidVolumeBounds>(jVolumeBounds);
    default:
      throw std::invalid_argument("Unknown volume bounds type!");
  }
  return nullptr;
}  // namespace Acts
