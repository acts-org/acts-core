// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include <Acts/Plugins/ActSVG/LayerSvgConverter.hpp>
#include <Acts/Plugins/ActSVG/TrackingGeometrySvgConverter.hpp>

namespace {

static inline Acts::Svg::Style layerStyle() {
  Acts::Svg::Style lStyle;
  lStyle.fillColor = {51, 153, 255};
  lStyle.fillOpacity = 0.75;
  lStyle.highlightColor = {255, 153, 51};
  lStyle.highlights = {"mouseover", "mouseout"};
  lStyle.strokeColor = {25, 25, 25};
  lStyle.strokeWidth = 0.5;
  lStyle.quarterSegments = 72u;

  return lStyle;
}

static inline Acts::Svg::Style infoStyle() {
  Acts::Svg::Style iStyle;
  iStyle.fillColor = {0, 0, 180};
  iStyle.fillOpacity = 0.8;
  iStyle.highlights = {};
  iStyle.fontSize = 40.;
  return iStyle;
}

static inline Acts::Svg::Style backgroundStyle() {
  Acts::Svg::Style bgStyle;
  bgStyle.fillColor = {55, 55, 55};
  bgStyle.fillOpacity = 0.50;
  bgStyle.highlights = {};
  bgStyle.strokeColor = {25, 25, 25};
  bgStyle.strokeWidth = 0.5;
  bgStyle.quarterSegments = 72u;
  return bgStyle;
}

static inline Acts::Svg::Style pointStyle() {
  Acts::Svg::Style pStyle;
  pStyle.fillColor = {200, 0, 0};
  pStyle.fillOpacity = 1.0;
  pStyle.highlightColor = {0, 200, 0};
  pStyle.highlights = {"mouseover", "mouseout"};
  pStyle.strokeColor = {0, 0, 0};
  pStyle.strokeWidth = 0.5;
  pStyle.quarterSegments = 72u;

  return pStyle;
}

static inline Acts::Svg::TrackingGeometryConverter::Options
trackingGeometryOptions() {
  Acts::GeometryIdentifier geoID(0);

  Acts::Svg::LayerConverter::Options lOptions;

  lOptions.name = "layer";
  lOptions.surfaceStyles =
      Acts::GeometryHierarchyMap<Acts::Svg::Style>({{geoID, layerStyle()}});

  Acts::Svg::TrackingGeometryConverter::Options tgOptions;
  tgOptions.prefix = "";
  tgOptions.layerOptions =
      Acts::GeometryHierarchyMap<Acts::Svg::LayerConverter::Options>(
          {{geoID, lOptions}});

  return tgOptions;
}

static inline Acts::Svg::TrackingGeometryConverter::Options
backgroundGeometryOptions() {
  Acts::GeometryIdentifier geoID(0);

  Acts::Svg::LayerConverter::Options lOptions;

  lOptions.name = "layer";
  lOptions.surfaceStyles = Acts::GeometryHierarchyMap<Acts::Svg::Style>(
      {{geoID, backgroundStyle()}});
  lOptions.moduleInfo = false;
  lOptions.gridInfo = false;
  lOptions.zRange = {-100, 100};
  lOptions.phiRange = {-0.2, 0.2};

  Acts::Svg::TrackingGeometryConverter::Options tgOptions;
  tgOptions.prefix = "";
  tgOptions.layerOptions =
      Acts::GeometryHierarchyMap<Acts::Svg::LayerConverter::Options>(
          {{geoID, lOptions}});

  return tgOptions;
}

}  // namespace

namespace ActsExamples {

static Acts::Svg::Style s_layerStyle = layerStyle();

static Acts::Svg::Style s_pointStyle = pointStyle();

static Acts::Svg::Style s_infoStyle = infoStyle();

static Acts::Svg::TrackingGeometryConverter::Options s_trackingGeometryOptions =
    trackingGeometryOptions();

static Acts::Svg::TrackingGeometryConverter::Options
    s_backgroundTrackingGeometryOptions = backgroundGeometryOptions();

}  // namespace ActsExamples
