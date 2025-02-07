// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/RangeXD.hpp"

#include <nlohmann/json.hpp>

// Custom Json encoder/decoders. Naming is mandated by nlohmann::json and thus
// can not match our naming guidelines.

namespace Acts {
class BinningData;

void to_json(nlohmann::json& j, const BinningData& bd);

void from_json(const nlohmann::json& j, BinningData& bd);

void to_json(nlohmann::json& j, const BinUtility& bu);

void from_json(const nlohmann::json& j, BinUtility& bu);

template <typename Type>
void to_json(nlohmann::json& j, const Range1D<Type>& r) {
  j["min"] = r.min();
  j["max"] = r.max();
}

template <typename Type>
void from_json(const nlohmann::json& j, Range1D<Type>& r) {
  r.setMin(static_cast<Type>(j["min"]));
  r.setMax(static_cast<Type>(j["max"]));
}

NLOHMANN_JSON_SERIALIZE_ENUM(AxisDirection,
                             {{AxisDirection::AxisX, "AxisX"},
                              {AxisDirection::AxisY, "AxisY"},
                              {AxisDirection::AxisZ, "AxisZ"},
                              {AxisDirection::AxisR, "AxisR"},
                              {AxisDirection::AxisPhi, "AxisPhi"},
                              {AxisDirection::AxisRPhi, "AxisRPhi"},
                              {AxisDirection::AxisTheta, "AxisTheta"},
                              {AxisDirection::AxisEta, "AxisEta"},
                              {AxisDirection::AxisMag, "AxisMag"}})

}  // namespace Acts
