// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
