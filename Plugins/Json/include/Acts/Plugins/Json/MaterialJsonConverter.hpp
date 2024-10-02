// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"

#include <nlohmann/json.hpp>

// Custom Json encoder/decoders. Naming is mandated by nlohmann::json and thus
// can not match our naming guidelines.
namespace Acts {

class ISurfaceMaterial;
class IVolumeMaterial;
class BinUtility;

using volumeMaterialPointer = const Acts::IVolumeMaterial*;
using surfaceMaterialPointer = const Acts::ISurfaceMaterial*;

void to_json(nlohmann::json& j, const Material& t);

void from_json(const nlohmann::json& j, Material& t);

void to_json(nlohmann::json& j, const MaterialSlab& t);

void from_json(const nlohmann::json& j, MaterialSlab& t);

void from_json(const nlohmann::json& j, MaterialSlabMatrix& t);

void to_json(nlohmann::json& j, const volumeMaterialPointer& material);

void from_json(const nlohmann::json& j, volumeMaterialPointer& material);

void to_json(nlohmann::json& j, const surfaceMaterialPointer& material);

void from_json(const nlohmann::json& j, surfaceMaterialPointer& material);

// This macro create a conversion for the mapping type enum
NLOHMANN_JSON_SERIALIZE_ENUM(Acts::MappingType,
                             {
                                 {Acts::MappingType::PreMapping, "PreMapping"},
                                 {Acts::MappingType::Default, "Default"},
                                 {Acts::MappingType::PostMapping,
                                  "PostMapping"},
                                 {Acts::MappingType::Sensor, "Sensor"},
                             })

namespace MaterialJsonConverter {

/// @brief Convert a surface material to json - detray format
///
/// @param surfaceMaterial is the surface material to be converted
/// @param surface is the surface the material is attached to
/// @param surfaceIndex is the index of the surface
/// @param gridLink [in, out] is the grid index in the volume
///
/// @note the surface is needed to shift the z boundaries for concentric cylinders
///
/// @return a json object representing the surface material in detray format
nlohmann::json toJsonDetray(const ISurfaceMaterial& material,
                            const Acts::Surface& surface,
                            std::size_t surfaceIndex,
                            std::map<std::size_t, std::size_t>& gridLink);

/// @brief Convert a bin utility to json - detray format
///
/// @param binUtility is the bin utility to be converted
/// @param surface is the surface the material is attached to
///
/// @note the surface is needed to shift the z boundaries for concentric cylinders
///
/// @return a json object representing the bin utility in detray format
nlohmann::json toJsonDetray(const Acts::BinUtility& binUtility,
                            const Acts::Surface& surface);

}  // namespace MaterialJsonConverter

}  // namespace Acts
