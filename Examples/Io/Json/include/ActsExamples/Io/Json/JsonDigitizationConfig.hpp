// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Plugins/Json/GeometryHierarchyMapJsonConverter.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/Digitization/SmearingConfig.hpp"

#include <algorithm>
#include <string>

#include <nlohmann/json.hpp>

namespace ActsExamples {

void to_json(nlohmann::json& j, const ParameterSmearingConfig& psc);

void from_json(const nlohmann::json& j, ParameterSmearingConfig& psc);

void to_json(nlohmann::json& j, const GeometricConfig& gdc);

void from_json(const nlohmann::json& j, GeometricConfig& gdc);

void to_json(nlohmann::json& j, const SmearingConfig& sdc);

void from_json(const nlohmann::json& j, SmearingConfig& sdc);

void to_json(nlohmann::json& j, const DigiComponentsConfig& dc);

void from_json(const nlohmann::json& j, DigiComponentsConfig& dc);

Acts::GeometryHierarchyMap<DigiComponentsConfig> readDigiConfigFromJson(
    const std::string& path);

void writeDigiConfigToJson(
    const Acts::GeometryHierarchyMap<DigiComponentsConfig>& cfg,
    const std::string& path);

using DigiConfigContainer = Acts::GeometryHierarchyMap<DigiComponentsConfig>;
using DigiConfigConverter =
    Acts::GeometryHierarchyMapJsonConverter<DigiComponentsConfig>;

}  // namespace ActsExamples
