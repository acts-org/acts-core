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
#include "Acts/Plugins/ActSVG/SurfaceSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "actsvg/core.hpp"
#include "actsvg/meta.hpp"

#include <map>
#include <memory>
#include <tuple>

namespace Acts {

class Surface;

namespace Experimental {
class Portal;
class DetectorVolume;
}  // namespace Experimental

namespace Svg {

using ProtoPortal = actsvg::proto::portal<std::vector<Vector3>>;
using ProtoLink = ProtoPortal::link;

namespace PortalConverter {

/// @brief Nested Options struct for conversion options
struct Options {
  /// The conversion options for the surface part
  SurfaceConverter::Options surfaceOptions;
  /// Link length
  ActsScalar linkLength = 10.;
  /// Link index map
  std::map<const Experimental::DetectorVolume*, unsigned int> volumeIndices;
};

/// Convert into a ProtoPortal
///
/// @param gtcx is the geometry context of the conversion call
/// @param portal is the detector portal to convert
/// @param portalOptions is the conversion options struct
///
/// @return a proto portal object
ProtoPortal convert(const GeometryContext& gctx,
                    const Experimental::Portal& portal,
                    const PortalConverter::Options& portalOptions);

}  // namespace PortalConverter

namespace View {

/// Convert into an acts::svg::object with an XY view
///
/// @param portal is the DetectorVolume portal (proto representation)
/// @param identification is the to be translated id_ for actsvg
///
/// @return an svg object that can be written out directly to disc
static inline actsvg::svg::object xy(const ProtoPortal& portal,
                                     const std::string& identification) {
  actsvg::views::x_y xyView;
  return actsvg::display::portal(identification, portal, xyView);
}

/// Convert into an acts::svg::object with an Zr view
///
/// @param portal is the DetectorVolume portal (proto representation)
/// @param identification is the to be translated id_ for actsvg
///
/// @return an svg object that can be written out directly to disc
static inline actsvg::svg::object zr(const ProtoPortal& portal,
                                     const std::string& identification) {
  actsvg::views::z_r zrView;
  return actsvg::display::portal(identification, portal, zrView);
}

}  // namespace View

}  // namespace Svg

}  // namespace Acts
