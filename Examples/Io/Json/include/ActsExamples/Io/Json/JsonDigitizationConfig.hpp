// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Plugins/Json/GeometryHierarchyMapJsonConverter.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/Digitization/SmearingConfig.hpp"

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

using DigiConfigConverter =
    Acts::GeometryHierarchyMapJsonConverter<DigiComponentsConfig>;

}  // namespace ActsExamples
