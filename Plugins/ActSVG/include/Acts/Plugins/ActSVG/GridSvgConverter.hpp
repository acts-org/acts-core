// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include <actsvg/core.hpp>
#include <actsvg/meta.hpp>

#include <array>
#include <optional>
#include <tuple>
#include <vector>

namespace Acts {

class Surface;

namespace Svg {

using ProtoGrid = actsvg::proto::grid;

namespace GridConverter {

// An optional range and binning value
using AxisBound = std::tuple<std::array<ActsScalar, 2u>, BinningValue>;

/// Nested Options struct
struct Options {
  /// A The style for the surfaces
  Style style{{150, 150, 150}, 0.5};
  /// Optional bound - in case of 1-dim grid
  std::optional<AxisBound> optionalBound;
};

/// Convert an ACTS grid into a actsvg protogrid, it currently works with
///
/// - 1D: [ binX ] , [ binY ], [ binR ] , [ binPhi ]
/// - 2D: [ binX, binY ], [ binZ, binPhi ], [ binR, binPhi ]
///
/// @tparam grid_type is the type of the grid to be converted
///
/// @param grid the grid to be converted
/// @param bValues the binning values identifying the axes
/// @param cOptions the conversion options
///
/// @return an ACTSVG proto grid for displaying
template <typename grid_type>
ProtoGrid convert(const grid_type& grid,
                  const std::array<BinningValue, grid_type::DIM>& bValues,
                  const GridConverter::Options& cOptions) {
  // The return object
  ProtoGrid pGrid;

  // Grid axes
  auto axes = grid.axes();

  // The edge values - these need to follow the ACTSVG convention,
  // so there could be swapping when necessary
  std::vector<Acts::ActsScalar> edges0;
  std::vector<Acts::ActsScalar> edges1;

  // 1D case (more to be filled in later)
  if constexpr (grid_type::DIM == 1u) {
    if (bValues[0u] == BinningValue::binPhi &&
        axes[0]->getBoundaryType() == AxisBoundaryType::Closed) {
      // swap     needed
      edges1 = axes[0]->getBinEdges();
      pGrid._type = actsvg::proto::grid::e_r_phi;
    }
    if (cOptions.optionalBound.has_value()) {
      auto [boundRange, boundValue] = cOptions.optionalBound.value();
      if (boundValue == BinningValue::binR) {
        // good - no swap needed
        edges0 = {boundRange[0u], boundRange[1u]};
      }
    }
  }
  // 2D cases
  if constexpr (grid_type::DIM == 2u) {
    // Assign
    edges0 = axes[0]->getBinEdges();
    edges1 = axes[1]->getBinEdges();
    if (bValues[0] == BinningValue::binPhi &&
        bValues[1] == BinningValue::binZ) {
      //  swap needed
      std::swap(edges0, edges1);
      pGrid._type = actsvg::proto::grid::e_z_phi;
    } else if (bValues[0] == BinningValue::binPhi &&
               bValues[1] == BinningValue::binR) {
      // swap needed
      std::swap(edges0, edges1);
      pGrid._type = actsvg::proto::grid::e_r_phi;
    } else if (bValues[0] == BinningValue::binZ &&
               bValues[1] == BinningValue::binPhi) {
      // good - no swap needed
      pGrid._type = actsvg::proto::grid::e_z_phi;
    } else if (bValues[0] == BinningValue::binR &&
               bValues[1] == BinningValue::binPhi) {
      // good - no swap needed
      pGrid._type = actsvg::proto::grid::e_r_phi;
    } else if (bValues[0] == BinningValue::binX &&
               bValues[1] == BinningValue::binY) {
      // good - no swap needed
      pGrid._type = actsvg::proto::grid::e_x_y;
    }
  }

  // Assign grid edges
  pGrid._edges_0 = std::vector<actsvg::scalar>(edges0.begin(), edges0.end());
  pGrid._edges_1 = std::vector<actsvg::scalar>(edges1.begin(), edges1.end());

  auto [fill, stroke] = cOptions.style.fillAndStroke();
  pGrid._fill = fill;
  pGrid._stroke = stroke;

  return pGrid;
}

}  // namespace GridConverter
}  // namespace Svg
}  // namespace Acts
