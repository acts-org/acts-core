// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
using AxisBound = std::tuple<std::array<double, 2u>, AxisDirection>;

/// Nested Options struct
struct Options {
  /// A The style for the surfaces
  Style style{{150, 150, 150}, 0.5};
  /// Optional bound - in case of 1-dim grid
  std::optional<AxisBound> optionalBound;
};

/// Convert an ACTS grid into a actsvg protogrid, it currently works with
///
/// - 1D: [ AxisX ] , [ AxisY ], [ AxisR ] , [ AxisPhi ]
/// - 2D: [ AxisX, AxisY ], [ AxisZ, AxisPhi ], [ AxisR, AxisPhi ]
///
/// @tparam grid_type is the type of the grid to be converted
///
/// @param grid the grid to be converted
/// @param aDirs the axis directions of the grid
/// @param cOptions the conversion options
///
/// @return an ACTSVG proto grid for displaying
template <typename grid_type>
ProtoGrid convert(const grid_type& grid,
                  const std::array<AxisDirection, grid_type::DIM>& aDirs,
                  const GridConverter::Options& cOptions) {
  // The return object
  ProtoGrid pGrid;

  // Grid axes
  auto axes = grid.axes();

  // The edge values - these need to follow the ACTSVG convention,
  // so there could be swapping when necessary
  std::vector<double> edges0;
  std::vector<double> edges1;

  // 1D case (more to be filled in later)
  if constexpr (grid_type::DIM == 1u) {
    if (aDirs[0u] == AxisDirection::AxisPhi &&
        axes[0]->getBoundaryType() == AxisBoundaryType::Closed) {
      // swap     needed
      edges1 = axes[0]->getBinEdges();
      pGrid._type = actsvg::proto::grid::e_r_phi;
    }
    if (cOptions.optionalBound.has_value()) {
      auto [boundRange, boundValue] = cOptions.optionalBound.value();
      if (boundValue == AxisDirection::AxisR) {
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
    if (aDirs[0] == AxisDirection::AxisPhi &&
        aDirs[1] == AxisDirection::AxisZ) {
      //  swap needed
      std::swap(edges0, edges1);
      pGrid._type = actsvg::proto::grid::e_z_phi;
    } else if (aDirs[0] == AxisDirection::AxisPhi &&
               aDirs[1] == AxisDirection::AxisR) {
      // swap needed
      std::swap(edges0, edges1);
      pGrid._type = actsvg::proto::grid::e_r_phi;
    } else if (aDirs[0] == AxisDirection::AxisZ &&
               aDirs[1] == AxisDirection::AxisPhi) {
      // good - no swap needed
      pGrid._type = actsvg::proto::grid::e_z_phi;
    } else if (aDirs[0] == AxisDirection::AxisR &&
               aDirs[1] == AxisDirection::AxisPhi) {
      // good - no swap needed
      pGrid._type = actsvg::proto::grid::e_r_phi;
    } else if (aDirs[0] == AxisDirection::AxisX &&
               aDirs[1] == AxisDirection::AxisY) {
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
