// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include <actsvg/core.hpp>
#include <actsvg/meta.hpp>

#include <numbers>

namespace Acts {

class Layer;

namespace Svg {

using ProtoVolume = actsvg::proto::volume<std::vector<Vector3>>;

static std::array<double, 2> noLimitZ = {std::numeric_limits<double>::lowest(),
                                         std::numeric_limits<double>::max()};

static std::array<double, 2> noLimitPhi = {-std::numbers::pi, std::numbers::pi};

namespace LayerConverter {

/// The enumeration for sheets
enum Sheets {
  eModuleInfo = 0,
  eGridInfo = 1,
  eCrossSectionXY = 2,
  eCrossSectionZR = 3
};

/// A nested options class for the layer conversion
struct Options {
  /// The name for the conversion object
  std::string name = "";
  /// The style of the surface objects
  GeometryHierarchyMap<Style> surfaceStyles;
  /// The z limit for projections
  std::array<double, 2> zRange = noLimitZ;
  /// The phi limit for projections
  std::array<double, 2> phiRange = noLimitPhi;
  /// Configuration of the views
  bool gridInfo = true;
  bool moduleInfo = true;
  bool projectionInfo = true;
  /// Label checks
  bool labelProjection = false;
  double labelGauge = 0.;
};

/// Write/create the layer sheets for a given layer
///
/// @param gctx the geometry context
/// @param layer the layer to be displayed
/// @param cOptions the conversion objects
///
/// @return a vector of svg objects
std::vector<actsvg::svg::object> convert(const GeometryContext& gctx,
                                         const Layer& layer,
                                         const Options& cOptions);

}  // namespace LayerConverter

}  // namespace Svg

}  // namespace Acts
