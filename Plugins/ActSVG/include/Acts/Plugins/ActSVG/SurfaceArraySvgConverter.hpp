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
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include <actsvg/meta.hpp>

#include <tuple>
#include <vector>

namespace Acts {

class SurfaceArray;

namespace Svg {

using ProtoSurface = actsvg::proto::surface<std::vector<Vector3>>;
using ProtoSurfaces = std::vector<ProtoSurface>;
using ProtoGrid = actsvg::proto::grid;
using ProtoAssociations = std::vector<std::vector<std::size_t>>;

namespace SurfaceArrayConverter {

/// Nested options struct
struct Options {
  /// Hierarchy map of styles
  GeometryHierarchyMap<Style> surfaceStyles;
};

/// Convert a surface array into needed constituents
///
/// @param gtcx is the geometry context of the conversion call
/// @param surfaceArray is the surface to convert
/// @param cOptions the conversion options
///
/// @note the type of view is auto-generated from the binning information
///
/// @return a collection of proto surface object and a grid, and associations
std::tuple<std::vector<ProtoSurfaces>, ProtoGrid,
           std::vector<ProtoAssociations>>
convert(const GeometryContext& gctx, const SurfaceArray& surfaceArray,
        const Options& cOptions);

}  // namespace SurfaceArrayConverter

}  // namespace Svg

}  // namespace Acts
