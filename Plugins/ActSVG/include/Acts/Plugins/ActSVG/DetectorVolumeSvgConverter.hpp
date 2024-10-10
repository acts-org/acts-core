// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Plugins/ActSVG/IndexedSurfacesSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/PortalSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include <actsvg/core.hpp>
#include <actsvg/meta.hpp>

#include <map>
#include <string>
#include <tuple>

namespace Acts {

namespace Experimental {
class DetectorVolume;
class Portal;
}  // namespace Experimental

namespace Svg {

using ProtoVolume = actsvg::proto::volume<std::vector<Vector3>>;

namespace DetectorVolumeConverter {

/// A nested options class for the layer conversion
struct Options {
  /// The Portal indices
  std::map<const Experimental::Portal*, unsigned int> portalIndices;
  /// The Portal converter options
  PortalConverter::Options portalOptions;
  /// The Surface converter options
  SurfaceConverter::Options surfaceOptions;
  /// The Grid converter options
  IndexedSurfacesConverter::Options indexedSurfacesOptions;
};

/// Write/create the detector volume
///
/// @param gctx the geometry context
/// @param dVolume the detector volumeto be displayed
/// @param volumeOptions the conversion objects
///
/// @return a vector of svg objects
std::tuple<ProtoVolume, ProtoIndexedSurfaceGrid> convert(
    const GeometryContext& gctx, const Experimental::DetectorVolume& dVolume,
    const Options& volumeOptions);

}  // namespace DetectorVolumeConverter

namespace View {

/// Convert into an acts::svg::object with an XY view
///
/// @param volume is the DetectorVolume (proto representation)
/// @param identification is the to be translated id_ for actsvg
/// @param displayPortals is a directory whether portals should be displayed
///
/// @return an svg object that can be written out directly to disc
static inline actsvg::svg::object xy(const ProtoVolume& volume,
                                     const std::string& identification,
                                     bool displayPortals = true) {
  actsvg::views::x_y xyView;
  return actsvg::display::volume(identification, volume, xyView,
                                 displayPortals);
}

/// Convert into an acts::svg::object with an Zr view
///
/// @param volume is the DetectorVolume (proto representation)
/// @param identification is the to be translated id_ for actsvg
/// @param displayPortals is a directory whether portals should be displayed
///
/// @return an svg object that can be written out directly to disc
static inline actsvg::svg::object zr(const ProtoVolume& volume,
                                     const std::string& identification,
                                     bool displayPortals = true) {
  actsvg::views::z_r zrView;
  return actsvg::display::volume(identification, volume, zrView,
                                 displayPortals);
}

}  // namespace View

}  // namespace Svg

}  // namespace Acts
