// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"

// Custom Json encoder/decoders. Naming is mandated by nlohman::json and thus
// can not match our naming guidelines.
namespace Acts {

using volumeMaterialPointer = const Acts::IVolumeMaterial*;
using surfaceMaterialPointer = const Acts::ISurfaceMaterial*;

void to_json(nlohmann::json& j, const Material& t);

void from_json(const nlohmann::json& j, Material& t);

void to_json(nlohmann::json& j, const MaterialSlab& t);

void from_json(const nlohmann::json& j, MaterialSlab& t);

void from_json(const nlohmann::json& j, MaterialSlabMatrix& t);

void to_json(nlohmann::json& j, const volumeMaterialPointer& t);

void from_json(const nlohmann::json& j, volumeMaterialPointer& t);

void to_json(nlohmann::json& j, const surfaceMaterialPointer& t);

void from_json(const nlohmann::json& j, surfaceMaterialPointer& t);

// This macro create a conversion for the mapping type enum
NLOHMANN_JSON_SERIALIZE_ENUM(Acts::MappingType,
                             {
                                 {Acts::MappingType::PreMapping, "PreMapping"},
                                 {Acts::MappingType::Default, "Default"},
                                 {Acts::MappingType::PostMapping,
                                  "PostMapping"},
                                 {Acts::MappingType::Sensor, "Sensor"},
                             })

}  // namespace Acts
