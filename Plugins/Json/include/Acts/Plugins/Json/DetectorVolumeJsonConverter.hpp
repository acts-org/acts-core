// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Plugins/Json/AlgebraJsonConverter.hpp"
#include "Acts/Plugins/Json/PortalJsonConverter.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"

#include <map>
#include <memory>
#include <vector>

// Custom Json encoder/decoders

namespace Acts {

class Surface;

namespace Experimental {
class DetectorVolume;
class Portal;
}  // namespace Experimental

namespace DetectorVolumeJsonConverter {

struct Options {
  // The options how surfaces are written out
  SurfaceJsonConverter::Options surfaceOptions;
  // The options how portals are written out
  PortalJsonConverter::Options portalOptions;
  // The options how transforms are written out
  Transform3JsonConverter::Options transformOptions;
};

/// @brief Convert to json format
///
/// @param gctx the geometry context
/// @param volume the detector volume instance
/// @param detectorVolumes the list of other detector volumes
/// @param portals the list of portals for saving the portal links
/// @param options the options for the conversion
///
/// @return a json object representing the detector volume
nlohmann::json toJson(
    const GeometryContext& gctx, const Experimental::DetectorVolume& volume,
    const std::vector<const Experimental::DetectorVolume*>& detectorVolumes,
    const std::vector<const Experimental::Portal*>& portals = {},
    const Options& options = Options{});

/// @brief convert from json format
///
/// @param gctx the geometry context
/// @param jVolume the json object representing the detector volume
///
/// @note this only creates a volume in a stand-alone context, not in a detector
///       context. For the latter, use the DetectorJsonConverter that will patch
///       the portals accordingly
std::shared_ptr<Experimental::DetectorVolume> fromJson(
    const GeometryContext& gctx, const nlohmann::json& jVolume);

}  // namespace DetectorVolumeJsonConverter
}  // namespace Acts
