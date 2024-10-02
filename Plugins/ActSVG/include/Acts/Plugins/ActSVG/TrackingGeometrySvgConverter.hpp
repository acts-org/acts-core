// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/ActSVG/LayerSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include <actsvg/core.hpp>
#include <actsvg/meta.hpp>

namespace Acts {

class TrackingGeometry;
class TrackingVolume;

namespace Svg {

namespace TrackingGeometryConverter {

/// Nested Options struct for the writing configuration
struct Options {
  /// Prefix the output names
  std::string prefix = "";
  /// Write the layer conversion options
  GeometryHierarchyMap<LayerConverter::Options> layerOptions;
};

/// State object to collect geometry-wise information
struct State {
  std::vector<actsvg::svg::object> xyCrossSection;
  std::vector<actsvg::svg::object> zrCrossSection;

  std::vector<actsvg::svg::object> finalViews;
};

/// Write svg files per tracking geometry
///
/// @param gctx the geometry context
/// @param tGeometry the tracking geometry
/// @param cOptions the conversion options
///
/// @return a vector of svg objects
std::vector<actsvg::svg::object> convert(const GeometryContext& gctx,
                                         const TrackingGeometry& tGeometry,
                                         const Options& cOptions);

/// Recursivele conversion of volumes
///
/// @param gctx the geometry context
/// @param tVolume the tracking volume
/// @param cOptions the conversion options
/// @param cState [in,out] the conversion state collecting the input
///
void convert(const GeometryContext& gctx, const TrackingVolume& tVolume,
             const Options& cOptions, State& cState);

}  // namespace TrackingGeometryConverter

namespace TrackingGeometryProjections {

struct Options {
  std::string prefix = "";

  TrackingGeometryConverter::Options trackingGeometryOptions;

  std::array<std::array<Acts::ActsScalar, 2>, 2> rzAxes;
  std::vector<ActsScalar> rzEtaLines;
};

/// Convert into xy and zr projections only
///
/// @param gctx the geometry context
/// @param tVolume the tracking volume
/// @param cOptions the conversion options
///
/// @note best performant if configured with options
/// that omit the module info and grid info
///
std::array<actsvg::svg::object, 2> convert(
    const GeometryContext& gctx, const Acts::TrackingGeometry& tGeometry,
    const Options& cOptions);
}  // namespace TrackingGeometryProjections

}  // namespace Svg

}  // namespace Acts
