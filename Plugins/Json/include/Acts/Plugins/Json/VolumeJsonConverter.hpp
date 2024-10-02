// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

// Custom Json encoder/decoders. Naming is mandated by nlohmann::json and thus
// can not match our naming guidelines.
namespace Acts {
class IVolumeMaterial;
class TrackingVolume;

/// Conversion of a pair of tracking volume and material used for the material
/// mapping
void to_json(
    nlohmann::json& j,
    const std::pair<const Acts::TrackingVolume*,
                    std::shared_ptr<const Acts::IVolumeMaterial>>& volume);

/// Conversion of a tracking volume
void to_json(nlohmann::json& j, const Acts::TrackingVolume& volume);

}  // namespace Acts
