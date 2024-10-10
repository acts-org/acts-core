// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ActSVG/LayerSvgConverter.hpp"

#include "Acts/Geometry/Layer.hpp"
#include "Acts/Plugins/ActSVG/SurfaceArraySvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SurfaceSvgConverter.hpp"

#include <set>
#include <sstream>

std::vector<actsvg::svg::object> Acts::Svg::LayerConverter::convert(
    const GeometryContext& gctx, const Layer& layer,
    const LayerConverter::Options& cOptions) {
  // The sheets
  std::vector<actsvg::svg::object> sheets;

  // The volume
  Acts::Svg::ProtoVolume volume;
  volume._name = cOptions.name;

  /// Convert the surface array into proto surfaces and a grid structure
  if (layer.surfaceArray() != nullptr) {
    SurfaceArrayConverter::Options sacOptions;
    sacOptions.surfaceStyles = cOptions.surfaceStyles;
    auto [surfaces, grid, associations] = SurfaceArrayConverter::convert(
        gctx, *(layer.surfaceArray()), sacOptions);
    volume._surfaces = surfaces;
    volume._surface_grid = grid;
    volume._grid_associations = associations;
  }

  // The sheet
  actsvg::svg::object module_sheet;
  actsvg::svg::object grid_sheet;
  actsvg::svg::object xy_layer;
  actsvg::svg::object zr_layer;

  // The module / grid information
  const auto& layerSurface = layer.surfaceRepresentation();
  if (layerSurface.type() == Acts::Surface::Disc) {
    if (cOptions.moduleInfo) {
      module_sheet = actsvg::display::endcap_sheet(
          cOptions.name + "_modules", volume, {800, 800},
          actsvg::display::e_module_info);
    }
    if (cOptions.gridInfo) {
      grid_sheet = actsvg::display::endcap_sheet(cOptions.name + "_grid",
                                                 volume, {800, 800},
                                                 actsvg::display::e_grid_info);
    }
  } else if (layerSurface.type() == Acts::Surface::Cylinder) {
    if (cOptions.moduleInfo) {
      module_sheet = actsvg::display::barrel_sheet(
          cOptions.name + "_modules", volume, {800, 800},
          actsvg::display::e_module_info);
    }
    if (cOptions.gridInfo) {
      grid_sheet = actsvg::display::barrel_sheet(cOptions.name + "_grid",
                                                 volume, {800, 800},
                                                 actsvg::display::e_grid_info);
    }
  }

  // The z_r view of things
  actsvg::views::z_r z_r_view;
  actsvg::views::x_y x_y_view;

  if (layer.surfaceArray() != nullptr) {
    // The x_y view of things
    xy_layer._tag = "g";
    xy_layer._id = cOptions.name + "_xy_view";
    // The x_r view of things
    zr_layer._tag = "g";
    zr_layer._id = cOptions.name + "_zr_view";
    unsigned int m = 0;
    // Potential labels
    Acts::ActsScalar avgRadius = 0.;

    for (const auto& sf : layer.surfaceArray()->surfaces()) {
      // Surface center
      const Acts::Vector3 rCenter =
          sf->binningPosition(gctx, Acts::BinningValue::binR);
      const Acts::Vector3 sfCenter = sf->center(gctx);
      Acts::ActsScalar radius = Acts::VectorHelpers::perp(rCenter);
      Acts::ActsScalar phi = Acts::VectorHelpers::phi(rCenter);
      Acts::ActsScalar z = sfCenter.z();
      // Get the average radius
      avgRadius += radius;
      // Raw display surfaces for projects
      actsvg::proto::surface<std::vector<Acts::Vector3>> projSurface;
      projSurface._vertices = sf->polyhedronRepresentation(gctx, 1u).vertices;
      // Draw only if they fall into the range restriction - for phi
      if (phi >= cOptions.phiRange[0] && phi <= cOptions.phiRange[1]) {
        std::string m_zr_id = std::string("zr_") + std::to_string(m++);
        zr_layer.add_object(Acts::Svg::View::zr(projSurface, m_zr_id));
      }
      // for z
      if (z >= cOptions.zRange[0] && z <= cOptions.zRange[1]) {
        std::string m_xy_id = std::string("xy_") + std::to_string(m++);
        xy_layer.add_object(Acts::Svg::View::xy(projSurface, m_xy_id));
      }
    }
    // Do the average
    avgRadius /= layer.surfaceArray()->surfaces().size();

    // Add a measure iuf requested
    if (cOptions.labelProjection) {
      ActsScalar xEnd = avgRadius * std::cos(cOptions.labelGauge);
      ActsScalar yEnd = avgRadius * std::sin(cOptions.labelGauge);
      xy_layer.add_object(measure(0., 0., xEnd, yEnd, "r", avgRadius, "mm"));
    }
  }

  // Register according to the enums
  sheets.push_back(module_sheet);
  sheets.push_back(grid_sheet);
  sheets.push_back(xy_layer);
  sheets.push_back(zr_layer);

  // Return the created sheets
  return sheets;
}
