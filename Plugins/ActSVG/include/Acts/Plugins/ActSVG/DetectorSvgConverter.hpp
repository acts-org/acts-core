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
#include "Acts/Plugins/ActSVG/DetectorVolumeSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include <actsvg/core.hpp>
#include <actsvg/meta.hpp>

namespace Acts {

namespace Experimental {
class Detector;
}

namespace Svg {

using ProtoDetector = actsvg::proto::detector<std::vector<Vector3>>;

namespace DetectorConverter {

/// A nested options class for the layer conversion
struct Options {
  /// Top level options
  DetectorVolumeConverter::Options volumeOptions;
};

/// Write/create the detector volume
///
/// @param gctx the geometry context
/// @param detector the detector volumeto be displayed
/// @param detectorOptions the conversion objects
///
/// @return a vector of svg objects
ProtoDetector convert(const GeometryContext& gctx,
                      const Experimental::Detector& detector,
                      const Options& detectorOptions);

}  // namespace DetectorConverter

namespace View {

/// Convert into an acts::svg::object with an XY view
///
/// @param detector is the detector (proto representation)
/// @param identification is the to be translated id_ for actsvg
///
/// @return an svg object that can be written out directly to disc
static inline actsvg::svg::object xy(const ProtoDetector& detector,
                                     const std::string& identification) {
  actsvg::views::x_y xyView;
  return actsvg::display::detector(identification, detector, xyView);
}

/// Convert into an acts::svg::object with an Zr view
///
/// @param detector is the detector (proto representation)
/// @param identification is the to be translated id_ for actsvg
///
/// @return an svg object that can be written out directly to disc
static inline actsvg::svg::object zr(const ProtoDetector& detector,
                                     const std::string& identification) {
  actsvg::views::z_r zrView;
  return actsvg::display::detector(identification, detector, zrView);
}

}  // namespace View

}  // namespace Svg

}  // namespace Acts
