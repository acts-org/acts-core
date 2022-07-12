// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/ActSVG/SurfaceSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"

#include <fstream>

BOOST_AUTO_TEST_SUITE(SurfaceSvgConverter)

namespace {

/// Helper to run planar Test
///
/// @param surface
/// @param identification
void runPlanarTests(const Acts::Surface& surface, const Acts::Svg::Style& style,
                    const std::string& identification) {
  // Default Geometry context
  Acts::GeometryContext geoCtx;

  // Svg proto object & actual object
  auto svgTemplate = Acts::Svg::convert(geoCtx, surface, style, true);
  auto xyTemplate =
      Acts::Svg::surfaceViewXY(svgTemplate, identification + "_template");
  Acts::Svg::toFile({xyTemplate}, xyTemplate._id + ".svg");
  // Positioned
  auto svgObject = Acts::Svg::convert(geoCtx, surface, style, false);
  auto xyObject = Acts::Svg::surfaceViewXY(svgObject, identification);
  Acts::Svg::toFile({xyObject}, xyObject._id + ".svg");
  // As sheet
  auto svgSheet = Acts::Svg::surfaceSheetXY(svgTemplate, identification + "_sheet");
  Acts::Svg::toFile({svgSheet}, svgSheet._id + ".svg");
};

}  // namespace

BOOST_AUTO_TEST_CASE(PlanarSurfaces) {
  // Planar style
  Acts::Svg::Style planarStyle;
  planarStyle.fillColor = {51, 153, 255};
  planarStyle.fillOpacity = 0.75;
  planarStyle.highlightColor = {255, 153, 51};
  planarStyle.highlights = {"mouseover", "mouseout"};
  planarStyle.strokeWidth = 0.5;
  planarStyle.nSegments = 0u;

  // Rectangle case
  auto rectangleBounds = std::make_shared<Acts::RectangleBounds>(36., 64.);
  auto transform = Acts::Transform3::Identity();
  transform.pretranslate(Acts::Vector3{20., 20., 100.});
  auto rectanglePlane =
      Acts::Surface::makeShared<Acts::PlaneSurface>(transform, rectangleBounds);
  runPlanarTests(*rectanglePlane, planarStyle, "rectangle");

  // Trapezoid case:
  auto trapezoidBounds =
      std::make_shared<Acts::TrapezoidBounds>(36., 64., 105.);
  auto trapeozidPlane =
      Acts::Surface::makeShared<Acts::PlaneSurface>(transform, trapezoidBounds);
  runPlanarTests(*trapeozidPlane, planarStyle, "trapezoid");

  // Diamond

  // ConvexPolygon
}

BOOST_AUTO_TEST_CASE(DiscSurfaces) {
  // Planar style
  Acts::Svg::Style discStyle;
  discStyle.fillColor = {0, 204, 153};
  discStyle.fillOpacity = 0.75;
  discStyle.highlightColor = {153, 204, 0};
  discStyle.highlights = {"mouseover", "mouseout"};
  discStyle.strokeWidth = 0.5;
  discStyle.nSegments = 72u;

  auto transform = Acts::Transform3::Identity();
  transform.pretranslate(Acts::Vector3{20., 20., 100.});

  // Full disc case
  auto fullDiscBounds = std::make_shared<Acts::RadialBounds>(0., 64.);
  auto fullDisc =
      Acts::Surface::makeShared<Acts::DiscSurface>(transform, fullDiscBounds);
  runPlanarTests(*fullDisc, discStyle, "full_disc");

  // Full ring case:
  auto fullRingBounds = std::make_shared<Acts::RadialBounds>(36., 64.);
  auto fullRing =
      Acts::Surface::makeShared<Acts::DiscSurface>(transform, fullRingBounds);
  runPlanarTests(*fullRing, discStyle, "full_ring");

  // Sectorial disc case
  auto sectoralDiscBounds =
      std::make_shared<Acts::RadialBounds>(0., 64., 0.25 * M_PI, 0.5 * M_PI);
  auto sectoralDisc = Acts::Surface::makeShared<Acts::DiscSurface>(
      transform, sectoralDiscBounds);
  runPlanarTests(*sectoralDisc, discStyle, "full_disc");

  // Annulus shape
  double minRadius = 7.2;
  double maxRadius = 12.0;
  double minPhi = 0.74195;
  double maxPhi = 1.33970;

  Acts::Vector2 offset{-3., 2.};

  auto annulusDiscBounds = std::make_shared<Acts::AnnulusBounds>(
      minRadius, maxRadius, minPhi, maxPhi, offset);
  auto annulusDisc = Acts::Surface::makeShared<Acts::DiscSurface>(
      transform, annulusDiscBounds);
  runPlanarTests(*annulusDisc, discStyle, "annulus_disc");
}

BOOST_AUTO_TEST_SUITE_END()
