// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/DiscLayer.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Plugins/ActSVG/LayerSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"

#include <fstream>
#include <memory>
#include <vector>

BOOST_AUTO_TEST_SUITE(ActSvg)

namespace {

Acts::GeometryContext tgContext;

std::shared_ptr<const Acts::LayerCreator> lCreator(nullptr);

void setupTools() {
  if (lCreator == nullptr) {
    Acts::LayerCreator::Config lCreatorCfg;
    lCreatorCfg.surfaceArrayCreator =
        std::make_shared<const Acts::SurfaceArrayCreator>();
    lCreator = std::make_shared<const Acts::LayerCreator>(lCreatorCfg);
  }
}

std::shared_ptr<Acts::Layer> generateDiscLayer(Acts::ActsScalar rInner,
                                               Acts::ActsScalar rOuter,
                                               unsigned int nSegments,
                                               unsigned int nRings,
                                               bool useTrapezoids = false) {
  // Some preparations
  setupTools();
  std::vector<std::shared_ptr<const Acts::Surface>> moduleSurfaces;
  Acts::ActsScalar phiStep = 2 * M_PI / nSegments;
  Acts::ActsScalar rStep = (rOuter - rInner) / nRings;
  // Reserve & fill
  moduleSurfaces.reserve(nSegments * nRings);
  // Radial disc
  if (!useTrapezoids) {
    for (unsigned int ir = 0; ir < nRings; ++ir) {
      std::shared_ptr<const Acts::RadialBounds> rBounds = nullptr;
      rBounds = std::make_shared<Acts::RadialBounds>(
          rInner + ir * rStep - 0.025 * rInner,
          rInner + (ir + 1u) * rStep + 0.025 * rInner, 0.55 * phiStep, 0.);
      for (unsigned int is = 0; is < nSegments; ++is) {
        // Place the module
        auto placement = Acts::Transform3::Identity();
        if ((is % 2) != 0u) {
          placement.pretranslate(Acts::Vector3{0., 0., 2.});
        }
        placement.rotate(
            Eigen::AngleAxisd(is * phiStep, Acts::Vector3(0, 0, 1)));
        auto dModule =
            Acts::Surface::makeShared<Acts::DiscSurface>(placement, rBounds);
        moduleSurfaces.push_back(dModule);
      }
    }
  } else {
    for (unsigned int ir = 0; ir < nRings; ++ir) {
      // Trapezoid parameters
      Acts::ActsScalar radius = rInner + (ir + 0.5) * rStep;
      Acts::ActsScalar yHalf = rStep * 0.5125;

      Acts::ActsScalar xHalfMin =
          1.15 * (rInner + ir * rStep) * M_PI / nSegments;
      Acts::ActsScalar xHalfMax =
          1.15 * (rInner + (ir + 1) * rStep) * M_PI / nSegments;

      std::shared_ptr<const Acts::TrapezoidBounds> tBounds =
          std::make_shared<const Acts::TrapezoidBounds>(xHalfMin, xHalfMax,
                                                        yHalf);
      for (unsigned int is = 0; is < nSegments; ++is) {
        // Setting the phi
        Acts::ActsScalar cphi = -M_PI + is * phiStep;
        Acts::Vector3 center(radius * std::cos(cphi), radius * std::sin(cphi),
                             (is % 2) * 2 + (ir % 2) * 5);
        // Local axis system
        Acts::Vector3 localY(std::cos(cphi), std::sin(cphi), 0.);
        Acts::Vector3 localZ(0., 0., 1.);
        Acts::Vector3 localX = localY.cross(localZ);
        Acts::RotationMatrix3 rotation;
        rotation.col(0) = localX;
        rotation.col(1) = localY;
        rotation.col(2) = localZ;
        Acts::Transform3 placement(Acts::Translation3(center) * rotation);
        // Create the module surface
        auto dModule =
            Acts::Surface::makeShared<Acts::PlaneSurface>(placement, tBounds);
        moduleSurfaces.push_back(dModule);
      }
    }
  }
  // Let's create the disc layer
  return lCreator->discLayer(tgContext, moduleSurfaces, nRings, nSegments);
}

}  // namespace

BOOST_AUTO_TEST_CASE(DiscLayerRadialSvg) {
  // Planar style
  Acts::Svg::Style discLayerStyle;
  discLayerStyle.fillColor = {51, 153, 255};
  discLayerStyle.fillOpacity = 0.75;
  discLayerStyle.highlightColor = {255, 153, 51};
  discLayerStyle.highlights = {"mouseover", "mouseout"};
  discLayerStyle.strokeColor = {25, 25, 25};
  discLayerStyle.strokeWidth = 0.5;
  discLayerStyle.nSegments = 72u;

  Acts::GeometryIdentifier geoID{0};

  // Get the layer
  auto discLayer = generateDiscLayer(100, 250, 32u, 4u);

  Acts::Svg::LayerConverter::Options lOptions;
  lOptions.name = "disc_layer_sectors";
  lOptions.surfaceStyles =
      Acts::GeometryHierarchyMap<Acts::Svg::Style>({{geoID, discLayerStyle}});

  // Get the layer sheets
  auto discLayerSheets =
      Acts::Svg::LayerConverter::convert(tgContext, *discLayer, lOptions);

  for (const auto& s : discLayerSheets) {
    Acts::Svg::toFile({s}, s._id + ".svg");
  }
}

BOOST_AUTO_TEST_CASE(DiscLayerTrapezoidSvg) {
  // Planar style
  Acts::Svg::Style discLayerStyle;
  discLayerStyle.fillColor = {51, 153, 255};
  discLayerStyle.fillOpacity = 0.75;
  discLayerStyle.highlightColor = {255, 153, 51};
  discLayerStyle.highlights = {"mouseover", "mouseout"};
  discLayerStyle.strokeColor = {25, 25, 25};
  discLayerStyle.strokeWidth = 0.5;
  discLayerStyle.nSegments = 72u;

  Acts::GeometryIdentifier geoID{0};

  // Get the layer
  auto discLayer = generateDiscLayer(100, 250, 32u, 4u, true);

  Acts::Svg::LayerConverter::Options lOptions;
  lOptions.name = "disc_layer_trapezoid";
  lOptions.surfaceStyles =
      Acts::GeometryHierarchyMap<Acts::Svg::Style>({{geoID, discLayerStyle}});

  // Get the layer sheets
  auto discLayerSheets =
      Acts::Svg::LayerConverter::convert(tgContext, *discLayer, lOptions);

  for (const auto& s : discLayerSheets) {
    Acts::Svg::toFile({s}, s._id + ".svg");
  }
}

BOOST_AUTO_TEST_CASE(CylinderLayerSvg) {
  //  Planar style
  Acts::Svg::Style cylinderLayerStyle;
  cylinderLayerStyle.fillColor = {51, 153, 255};
  cylinderLayerStyle.fillOpacity = 0.75;
  cylinderLayerStyle.highlightColor = {255, 153, 51};
  cylinderLayerStyle.highlights = {"mouseover", "mouseout"};
  cylinderLayerStyle.strokeColor = {25, 25, 25};
  cylinderLayerStyle.strokeWidth = 0.5;
  cylinderLayerStyle.nSegments = 72u;

  Acts::GeometryIdentifier geoID{0};

  Acts::Test::CylindricalTrackingGeometry cGeometry(tgContext);
  auto tGeometry = cGeometry();
  auto pixelVolume =
      tGeometry->lowestTrackingVolume(tgContext, Acts::Vector3(50., 0., 0.));
  if (pixelVolume != nullptr && pixelVolume->confinedLayers() != nullptr) {
    auto layers = pixelVolume->confinedLayers()->arrayObjects();
    std::size_t il = 0;
    for (const auto& layer : layers) {
      if (layer->surfaceArray() != nullptr) {
        continue;
      }

      Acts::Svg::LayerConverter::Options lOptions;
      lOptions.name = "cylinder_layer_" + std::to_string(il++);
      lOptions.surfaceStyles = Acts::GeometryHierarchyMap<Acts::Svg::Style>(
          {{geoID, cylinderLayerStyle}});

      // Get the layer sheets
      auto layerSheets =
          Acts::Svg::LayerConverter::convert(tgContext, *layer, lOptions);
      for (const auto& s : layerSheets) {
        Acts::Svg::toFile({s}, s._id + ".svg");
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(PlaeyLayerSvg) {}

BOOST_AUTO_TEST_SUITE_END()
