// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Navigation/PortalNavigation.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <tuple>
#include <vector>

// Custom Json encoder/decoders

namespace Acts {

class Surface;

namespace Experimental {
class DetectorVolume;
class Portal;
}  // namespace Experimental

namespace PortalJsonConverter {

struct Options {
  /// Options how to write the surface out
  SurfaceJsonConverter::Options surfaceOptions =
      SurfaceJsonConverter::Options{};
};

/// @brief Convert to json format
///
/// @param gctx the geometry context
/// @param portal the portal instance
/// @param detectorVolumes is the list of all detector voluems for portal pointing
/// @param options how to write this thing out
///
/// @return a json object
nlohmann::json toJson(
    const GeometryContext& gctx, const Experimental::Portal& portal,
    const std::vector<const Experimental::DetectorVolume*>& detectorVolumes,
    const Options& options = Options{});

/// @brief Convert to json format - dedicated Detray function
///
/// @param gctx the geometry context
/// @param portal the portal instance
/// @param ip is the portal index that could be used to pick the oriented surface
/// @param volume is the detector volume to which these surfaces belong to
/// @param orientedSurfaces are the bounding surfaces (may still need to be split)
/// @param detectorVolumes is the list of all detector voluems for portal pointing
/// @param options how to write this thing out
///
/// @note that detray relies on singly connected masks, hence a portal from ACTS
/// with multi volume link needs to be split into the multiple volumes
///
/// @note detray also only has outside pointing links
///
/// @return a tuple of json object
std::tuple<std::vector<nlohmann::json>, std::vector<std::shared_ptr<Surface>>>
toJsonDetray(
    const GeometryContext& gctx, const Experimental::Portal& portal,
    std::size_t ip, const Experimental::DetectorVolume& volume,
    const std::vector<OrientedSurface>& orientedSurfaces,
    const std::vector<const Experimental::DetectorVolume*>& detectorVolumes,
    const Options& options = Options{});

/// @brief convert to json format
/// @param updator the detector volume updator
/// @param detectorVolumes the list of all detector volumes
///
/// @return a json object
nlohmann::json toJson(
    const Experimental::ExternalNavigationDelegate& updator,
    const std::vector<const Experimental::DetectorVolume*>& detectorVolumes);

/// @brief convert from json format
///
/// @param gctx the geometry context
/// @param jPortal is the json portal description
/// @param detectorVolumes is the list of already created detector volumes
///
/// @return a Portal
std::shared_ptr<Experimental::Portal> fromJson(
    const GeometryContext& gctx, const nlohmann::json& jPortal,
    const std::vector<std::shared_ptr<Experimental::DetectorVolume>>&
        detectorVolumes);

}  // namespace PortalJsonConverter
}  // namespace Acts
