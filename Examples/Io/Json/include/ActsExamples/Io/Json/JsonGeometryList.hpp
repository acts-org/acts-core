// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
