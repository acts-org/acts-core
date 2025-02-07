// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Detector/ProtoDetector.hpp"
#include "Acts/Plugins/Json/UtilitiesJsonConverter.hpp"

#include <nlohmann/json.hpp>

// Custom Json encoder/decoders. Naming is mandated by nlohmann::json and thus
// can not match our naming guidelines.

namespace Acts {
struct ProtoDetector;
struct ProtoVolume;

void to_json(nlohmann::json& j, const ProtoVolume& pv);

void from_json(const nlohmann::json& j, ProtoVolume& pv);

void to_json(nlohmann::json& j, const ProtoDetector& pd);

void from_json(const nlohmann::json& j, ProtoDetector& pd);

}  // namespace Acts
