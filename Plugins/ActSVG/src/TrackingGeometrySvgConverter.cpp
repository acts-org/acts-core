// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ActSVG/TrackingGeometrySvgConverter.hpp"

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"

std::vector<actsvg::svg::object> Acts::Svg::TrackingGeometryConverter::convert(
    const GeometryContext& gctx, const TrackingGeometry& tGeometry,
    const TrackingGeometryConverter::Options& cOptions) {
  // Get the world volume
  const TrackingVolume* world = tGeometry.highestTrackingVolume();

  // Initiate the cache
  TrackingGeometryConverter::State cState;

  // Run the conversion recursively
  convert(gctx, *world, cOptions, cState);

  // Digest the views and globals
  std::vector<actsvg::svg::object> finalViews = cState.finalViews;
  if (!cState.xyCrossSection.empty()) {
    finalViews.push_back(
        Acts::Svg::group(cState.xyCrossSection, cOptions.prefix + "layers_xy"));
  }
  if (!cState.zrCrossSection.empty()) {
    finalViews.push_back(
        Acts::Svg::group(cState.zrCrossSection, cOptions.prefix + "layers_zr"));
  }
  // return all final Views
  return finalViews;
}

void Acts::Svg::TrackingGeometryConverter::convert(
    const GeometryContext& gctx, const TrackingVolume& tVolume,
    const TrackingGeometryConverter::Options& cOptions,
    TrackingGeometryConverter::State& cState) {
  // Process confined layers first
  if (tVolume.confinedLayers() != nullptr) {
    for (const auto& layer : tVolume.confinedLayers()->arrayObjects()) {
      if (layer->surfaceArray() != nullptr) {
        GeometryIdentifier geoID = layer->geometryId();
        std::string layerName = cOptions.prefix + "vol_" +
                                std::to_string(geoID.volume()) + "_layer_" +
                                std::to_string(geoID.layer());

        LayerConverter::Options lOptions;
        // Search for predefined layer options
        auto deflOptions = cOptions.layerOptions.find(geoID);
        if (deflOptions != cOptions.layerOptions.end()) {
          lOptions = (*deflOptions);
          lOptions.name = layerName;
        }
        // Retrieve the layer sheets
        auto layerSheets = LayerConverter::convert(gctx, *layer, lOptions);

        // Record the sheets
        for (const auto& lSheet : layerSheets) {
          if (lSheet.is_defined()) {
            cState.finalViews.push_back(lSheet);
          }
        }
        // Collect the xy views
        if (layerSheets[LayerConverter::eCrossSectionXY].is_defined() &&
            layer->surfaceRepresentation().type() == Acts::Surface::Cylinder) {
          cState.xyCrossSection.push_back(
              layerSheets[LayerConverter::eCrossSectionXY]);
        }
        // Collect the zr views
        if (layerSheets[LayerConverter::eCrossSectionZR].is_defined()) {
          cState.zrCrossSection.push_back(
              layerSheets[LayerConverter::eCrossSectionZR]);
        }
      }
    }
  }

  // Run recursively over confined volumes
  if (tVolume.confinedVolumes() != nullptr) {
    for (const auto& volume : tVolume.confinedVolumes()->arrayObjects()) {
      convert(gctx, *volume, cOptions, cState);
    }
  }
}

std::array<actsvg::svg::object, 2>
Acts::Svg::TrackingGeometryProjections::convert(
    const GeometryContext& gctx, const Acts::TrackingGeometry& tGeometry,
    const TrackingGeometryProjections::Options& cOptions) {
  // The projections
  actsvg::svg::object xyView;
  actsvg::svg::object zrView;

  // Get the world volume
  const Acts::TrackingVolume* world = tGeometry.highestTrackingVolume();
  if (world != nullptr) {
    // Initiate the cache
    Acts::Svg::TrackingGeometryConverter::State cState;

    // Run the conversion recursively
    Acts::Svg::TrackingGeometryConverter::convert(
        gctx, *world, cOptions.trackingGeometryOptions, cState);

    xyView = Acts::Svg::group(cState.xyCrossSection,
                              cOptions.prefix + "projection_xy");
    zrView = Acts::Svg::group(cState.zrCrossSection,
                              cOptions.prefix + "projection_zr");
  }
  return {xyView, zrView};
}
