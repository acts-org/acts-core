// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Plugins/Json/DetectorVolumeJsonConverter.hpp"

// Custom Json encoder/decoders

namespace Acts {

namespace Experimental {
class Detector;
}

namespace DetectorJsonConverter {

struct Options {
  DetectorVolumeJsonConverter::Options volumeOptions =
      DetectorVolumeJsonConverter::Options{};
};

/// @brief Convert to json format
///
/// @param gctx the geometry context
/// @param detector the detector instance
/// @param options the writing options that propagate
///        to the downstream converters
///
/// @return a json object
nlohmann::json toJson(const GeometryContext& gctx,
                      const Experimental::Detector& detector,
                      const Options& options = Options{});

/// @brief Convert to detray json format
///
/// @param gctx the geometry context
/// @param detector the detector instance
/// @param options the writing options that propagate
///        to the downstream converters
///
/// @return a json object in detray format
nlohmann::json toJsonDetray(const GeometryContext& gctx,
                            const Experimental::Detector& detector,
                            const Options& options = Options{});

/// @brief convert from json format
///
/// @param gctx the geometry context
/// @param jDetector the json object
///
/// @return a newly created shared Detector object
std::shared_ptr<Experimental::Detector> fromJson(
    const GeometryContext& gctx, const nlohmann::json& jDetector);

}  // namespace DetectorJsonConverter
}  // namespace Acts
