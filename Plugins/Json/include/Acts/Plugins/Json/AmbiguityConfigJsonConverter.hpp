// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/AmbiguityResolution/ScoreBasedAmbiguityResolution.hpp"

#include <map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace Acts {

using DetectorConfig = ScoreBasedAmbiguityResolution::DetectorConfig;
using ConfigPair =
    std::pair<std::map<std::size_t, std::size_t>, std::vector<DetectorConfig>>;
/// This function is used to convert the ScoreBasedAmbiguityResolutionCongig
/// from JSON to C++
void from_json(const nlohmann::json& j, ConfigPair& p);

}  // namespace Acts
