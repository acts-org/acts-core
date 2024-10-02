// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include <actsvg/core.hpp>
#include <actsvg/meta.hpp>

namespace Acts {

class Surface;

namespace Svg {

using ProtoSurface = actsvg::proto::surface<std::vector<Vector3>>;

namespace SurfaceConverter {

/// Nested Options struct
struct Options {
  /// A The style for the surfaces
  Style style;
  /// Indicate if you want to draw this as a template surface
  bool templateSurface = false;
};

/// Convert into a svg::proto surface
///
/// @param gtcx is the geometry context of the conversion call
/// @param surface is the surface to convert
/// @param cOption is the conversion options struct
///
/// @return a proto surface object
ProtoSurface convert(const GeometryContext& gctx, const Surface& surface,
                     const SurfaceConverter::Options& cOptions);

}  // namespace SurfaceConverter

namespace View {

/// Convert into an acts::svg::object with an x-y view
///
/// @param pSurface is the proto object
/// @param identification is the to be translated id_ for actsvg
///
/// @return an svg object that can be written out directly to disc
static inline actsvg::svg::object xy(const ProtoSurface& pSurface,
                                     const std::string& identification) {
  actsvg::views::x_y xyView;
  return actsvg::display::surface(identification, pSurface, xyView);
}

/// Convert into an acts::svg::object with an z-r view
///
/// @param pSurface is the proto object
/// @param identification is the to be translated id_ for actsvg
///
/// @return an svg object that can be written out directly to disc
static inline actsvg::svg::object zr(const ProtoSurface& pSurface,
                                     const std::string& identification) {
  actsvg::views::z_r zrView;
  return actsvg::display::surface(identification, pSurface, zrView);
}

/// Convert into an acts::svg::object with an z-phi view
///
/// @param pSurface is the proto object
/// @param identification is the to be translated id_ for actsvg
///
/// @return an svg object that can be written out directly to disc
static inline actsvg::svg::object zphi(const ProtoSurface& pSurface,
                                       const std::string& identification) {
  actsvg::views::z_phi zphiView;
  return actsvg::display::surface(identification, pSurface, zphiView);
}

/// Convert into an acts::svg::object with an z-rphi view
///
/// @param pSurface is the proto object
/// @param identification is the to be translated id_ for actsvg
///
/// @note it captures the radii[0u] element for plotting
///
/// @return an svg object that can be written out directly to disc
static inline actsvg::svg::object zrphi(const ProtoSurface& pSurface,
                                        const std::string& identification) {
  actsvg::views::z_rphi zrphiView;
  zrphiView._fixed_r = pSurface._radii[0u];
  return actsvg::display::surface(identification, pSurface, zrphiView);
}

}  // namespace View

namespace Sheet {

/// Convert into an acts::svg::object with an XY sheet
///
/// @param pSurface is the proto object
/// @param identification is the to be translated id_ for actsvg
///
/// @return an svg object that can be written out directly to disc
static inline actsvg::svg::object xy(const ProtoSurface& pSurface,
                                     const std::string& identification) {
  return actsvg::display::surface_sheet_xy(identification, pSurface);
}

}  // namespace Sheet

}  // namespace Svg

}  // namespace Acts
