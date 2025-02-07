// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace ActsExamples {

void from_json(const nlohmann::json& data, Acts::GeometryIdentifier& geoId);

void to_json(nlohmann::json& data, const Acts::GeometryIdentifier& geoId);

void from_json(const nlohmann::json& data,
               std::vector<Acts::GeometryIdentifier>& geoIdList);

void to_json(nlohmann::json& data,
             const std::vector<Acts::GeometryIdentifier>& geoIdList);

std::vector<Acts::GeometryIdentifier> readJsonGeometryList(
    const std::string& path);

void writeJsonGeometryList(
    const std::vector<Acts::GeometryIdentifier>& geoIdList,
    const std::string& path);

}  // namespace ActsExamples
