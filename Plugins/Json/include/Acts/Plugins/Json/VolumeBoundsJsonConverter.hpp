// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GenericCuboidVolumeBounds.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// Custom Json encoder/decoders. Naming is mandated by nlohmann::json and thus
// can not match our naming guidelines.
namespace Acts {

void to_json(nlohmann::json& j, const VolumeBounds& bounds);

namespace VolumeBoundsJsonConverter {

/// Conversion to Json from volume bounds
///
/// @param bounds is the bounds object
///
/// @return the json object
nlohmann::json toJson(const VolumeBounds& bounds);

/// Conversion to volume bounds from json
///
/// The type is given as a template argument in order to be able
/// to construct the correct fitting types for surfaces.
///
/// @param jVolumeBounds the read-in json object
///
/// @return a unique_ptr to a volume bounds object for type polymorphism
template <typename bounds_t>
std::unique_ptr<bounds_t> fromJson(const nlohmann::json& jVolumeBounds) {
  constexpr std::size_t kValues = bounds_t::BoundValues::eSize;
  std::array<double, kValues> bValues{};
  std::vector<double> bVector = jVolumeBounds["values"];
  std::copy_n(bVector.begin(), kValues, bValues.begin());
  return std::make_unique<bounds_t>(bValues);
}

/// Conversion to volume bounds from json
/// @param jVolumeBounds the read-in json object
///
/// @return a unique_ptr to a volume bounds object for type polymorphism
std::unique_ptr<VolumeBounds> fromJson(const nlohmann::json& jVolumeBounds);

}  // namespace VolumeBoundsJsonConverter

// This macro creates a conversion for the volume bounds type
NLOHMANN_JSON_SERIALIZE_ENUM(
    VolumeBounds::BoundsType,
    {{VolumeBounds::BoundsType::eCone, "Cone"},
     {VolumeBounds::BoundsType::eCuboid, "Cuboid"},
     {VolumeBounds::BoundsType::eCutoutCylinder, "CutoutCylinder"},
     {VolumeBounds::BoundsType::eCylinder, "Cylinder"},
     {VolumeBounds::BoundsType::eGenericCuboid, "GenericCuboid"},
     {VolumeBounds::BoundsType::eTrapezoid, "Trapezoid"}})

}  // namespace Acts
