// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/ActSVG/SurfaceSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/ConvexPolygonBounds.hpp"
#include "Acts/Surfaces/DiamondBounds.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"

#include <fstream>

BOOST_AUTO_TEST_SUITE(ActSvg)

namespace {

/// Helper to run planar Test
///
/// @param surface
/// @param identification
void runPlanarTests(const Acts::Surface& surface, const Acts::Svg::Style& style,
                    const std::string& identification) {
  // Default Geometry context
  Acts::GeometryContext geoCtx;

  using SurfaceOptions = Acts::Svg::SurfaceConverter::Options;

  SurfaceOptions sOptions;
  sOptions.style = style;
  sOptions.templateSurface = true;

  // Svg proto object & actual object
  auto svgTemplate =
      Acts::Svg::SurfaceConverter::convert(geoCtx, surface, sOptions);
  auto xyTemplate =
      Acts::Svg::View::xy(svgTemplate, identification + "_template");
  Acts::Svg::toFile({xyTemplate}, xyTemplate._id + ".svg");
  // Positioned
  sOptions.templateSurface = false;
  auto svgObject =
      Acts::Svg::SurfaceConverter::convert(geoCtx, surface, sOptions);
  auto xyObject = Acts::Svg::View::xy(svgObject, identification);
  auto xyAxes =
      Acts::Svg::axesXY(static_cast<Acts::ActsScalar>(xyObject._x_range[0]),
                        static_cast<Acts::ActsScalar>(xyObject._x_range[1]),
                        static_cast<Acts::ActsScalar>(xyObject._y_range[0]),
                        static_cast<Acts::ActsScalar>(xyObject._y_range[1]));

  Acts::Svg::toFile({xyObject, xyAxes}, xyObject._id + ".svg");
  // As sheet
  auto svgSheet = Acts::Svg::Sheet::xy(svgTemplate, identification + "_sheet");
  Acts::Svg::toFile({svgSheet}, svgSheet._id + ".svg");
}

}  // namespace

BOOST_AUTO_TEST_CASE(PlanarSurfaces) {
  // Planar style
  Acts::Svg::Style planarStyle;
  planarStyle.fillColor = {51, 153, 255};
  planarStyle.fillOpacity = 0.75;
  planarStyle.highlightColor = {255, 153, 51};
  planarStyle.highlights = {"mouseover", "mouseout"};
  planarStyle.strokeWidth = 0.5;
  planarStyle.quarterSegments = 0u;

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

  // Trapezoid case shifted and rotated
  Acts::ActsScalar phi = 0.125 * M_PI;
  Acts::ActsScalar radius = 150.;
  Acts::Vector3 center(radius * std::cos(phi), radius * std::sin(phi), 0.);

  Acts::Vector3 localY(std::cos(phi), std::sin(phi), 0.);
  Acts::Vector3 localZ(0., 0., 1.);
  Acts::Vector3 localX = localY.cross(localZ);
  Acts::RotationMatrix3 rotation;
  rotation.col(0) = localX;
  rotation.col(1) = localY;
  rotation.col(2) = localZ;
  transform = Acts::Transform3(Acts::Translation3(center) * rotation);
  // Create the module surface
  auto trapeozidPlaneTransformed =
      Acts::Surface::makeShared<Acts::PlaneSurface>(transform, trapezoidBounds);

  runPlanarTests(*trapeozidPlaneTransformed, planarStyle, "trapezoid_rotated");
  // A reference test for the rotated one
  Acts::GeometryContext geoCtx;
  actsvg::proto::surface<std::vector<Acts::Vector3>> reference;
  reference._vertices =
      trapeozidPlaneTransformed->polyhedronRepresentation(geoCtx, 1u).vertices;
  auto referenceTrapezoid = Acts::Svg::View::xy(reference, "trapezoid");
  auto referenceAxes = Acts::Svg::axesXY(-200., 200., -200., 200);
  Acts::Svg::toFile({referenceTrapezoid, referenceAxes},
                    "trapezoid_reference.svg");

  // Let's create one with a flipped z-axis
  Acts::Vector3 flocalZ(0., 0., -1.);
  Acts::Vector3 flocalX = localY.cross(flocalZ);
  Acts::RotationMatrix3 frotation;
  frotation.col(0) = flocalX;
  frotation.col(1) = localY;
  frotation.col(2) = flocalZ;
  auto ftransform = Acts::Transform3(Acts::Translation3(center) * frotation);
  // Create the module surface
  auto ftrapeozidPlaneTransformed =
      Acts::Surface::makeShared<Acts::PlaneSurface>(ftransform,
                                                    trapezoidBounds);

  runPlanarTests(*ftrapeozidPlaneTransformed, planarStyle,
                 "flipped_trapezoid_rotated");
  actsvg::proto::surface<std::vector<Acts::Vector3>> freference;
  freference._vertices =
      ftrapeozidPlaneTransformed->polyhedronRepresentation(geoCtx, 1u).vertices;

  auto freferenceTrapezoid =
      Acts::Svg::View::xy(freference, "flipped_trapezoid");
  Acts::Svg::toFile({freferenceTrapezoid, referenceAxes},
                    "flipped_trapezoid_reference.svg");

  // Diamond
  auto diamondBounds =
      std::make_shared<Acts::DiamondBounds>(36., 64., 14., 40., 30.);
  transform = Acts::Transform3::Identity();
  auto diamond =
      Acts::Surface::makeShared<Acts::PlaneSurface>(transform, diamondBounds);
  runPlanarTests(*diamond, planarStyle, "diamond");

  // ConvexPolygon
  std::vector<Acts::Vector2> vertices = {
      {-10., -10.}, {10., -15.}, {20., 5.}, {-5., 15.}, {-12, 0.}};
  auto polygonBounds =
      std::make_shared<Acts::ConvexPolygonBounds<5u>>(vertices);
  auto polygon =
      Acts::Surface::makeShared<Acts::PlaneSurface>(transform, polygonBounds);
  runPlanarTests(*polygon, planarStyle, "polygon");
}

BOOST_AUTO_TEST_CASE(DiscSurfaces) {
  // Planar style
  Acts::Svg::Style discStyle;
  discStyle.fillColor = {0, 204, 153};
  discStyle.fillOpacity = 0.75;
  discStyle.highlightColor = {153, 204, 0};
  discStyle.highlights = {"mouseover", "mouseout"};
  discStyle.strokeWidth = 0.5;
  discStyle.quarterSegments = 72u;

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
