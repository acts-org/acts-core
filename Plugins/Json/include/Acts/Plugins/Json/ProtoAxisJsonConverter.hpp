// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Utilities/ProtoAxis.hpp"

#include <nlohmann/json.hpp>

/// Custom Json encoder/decoders. Naming is mandated by nlohmann::json and thus
/// can not match our naming guidelines.
///
/// This uses a custom API and nomenclature as it would
/// otherwise require the ProtoAxis to have a default
/// constructor which is deleted
namespace Acts::ProtoAxisJsonConverter {

/// Write the ProtoAxis to a json object
///
/// @param pa the proto axis to be written out
nlohmann::json toJson(const ProtoAxis& pa);

/// Create a ProtoAxis from a json object
///
/// @param j the json object to be read from
Acts::ProtoAxis fromJson(const nlohmann::json& j);

}  // namespace Acts::ProtoAxisJsonConverter
