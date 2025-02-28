// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/detail/IndexedGridFiller.hpp"
#include "Acts/Detector/detail/IndexedSurfacesGenerator.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Plugins/ActSVG/IndexedSurfacesSvgConverter.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"

#include <numbers>
#include <tuple>

using namespace Acts;
using namespace Acts::Svg;
using namespace Acts::Test;
using namespace Acts::Experimental;
using namespace Acts::Experimental::detail;

GeometryContext tContext;
CylindricalTrackingGeometry cGeometry = CylindricalTrackingGeometry(tContext);

IndexedSurfacesConverter::Options generateDrawOptions() {
  // The converter options
  IndexedSurfacesConverter::Options isOptions;
  // Sensitive surface style
  Style sensitiveStyle;
  sensitiveStyle.fillColor = {51, 153, 255};
  sensitiveStyle.fillOpacity = 0.9;
  sensitiveStyle.highlightColor = {255, 153, 51};
  sensitiveStyle.highlights = {"onmouseover", "onmouseout"};
  sensitiveStyle.strokeWidth = 0.5;
  sensitiveStyle.strokeColor = {0, 0, 0};
  sensitiveStyle.quarterSegments = 72u;
  std::pair<GeometryIdentifier, Style> allSensitives = {GeometryIdentifier(0u),
                                                        sensitiveStyle};

  // Hierarchy map of styles
  GeometryHierarchyMap<Style> surfaceStyles({allSensitives});
  isOptions.surfaceStyles = surfaceStyles;

  // The grid style
  GridConverter::Options gridOptions;
  Style gridStyle;
  gridStyle.fillOpacity = 0.;
  gridStyle.strokeColor = {0, 0, 255};
  gridStyle.strokeWidth = 1.;
  gridStyle.highlightStrokeWidth = 3;
  gridStyle.highlightStrokeColor = {255, 0, 0};
  gridOptions.style = gridStyle;

  isOptions.gridOptions = gridOptions;
  return isOptions;
}

auto drawOptions = generateDrawOptions();

BOOST_AUTO_TEST_SUITE(ActSvg)

BOOST_AUTO_TEST_CASE(RingDisc1D) {
  // A single ring
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfaces = cGeometry.surfacesRing(dStore, 6.4, 12.4, 36., 0.125, 0.,
                                          55., 0., 2., 22u);
  // Polyhedron reference generator
  PolyhedronReferenceGenerator<1u, true> rGenerator;
  // A single proto axis clused in phi with 44 bins
  ProtoAxis pAxis(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                  -std::numbers::pi, std::numbers::pi, 44u);
  auto indexedRing =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          Experimental::IndexedSurfacesNavigation>(tContext, rSurfaces,
                                                   rGenerator, pAxis, 0u);
  // The displaying
  auto pIndexedRing = IndexedSurfacesConverter::convert(
      tContext, rSurfaces, indexedRing, drawOptions);
  auto pIndexRingView = View::xy(pIndexedRing, "RingDisc1D");
  toFile({pIndexRingView}, pIndexRingView._id + ".svg");
}

BOOST_AUTO_TEST_CASE(RingDisc1DWithSupport) {
  // A single ring
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfaces = cGeometry.surfacesRing(dStore, 6.4, 12.4, 36., 0.125, 0.,
                                          55., 0., 2., 22u);

  auto rBounds = std::make_shared<RadialBounds>(10., 20.);
  auto dSurface = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                   std::move(rBounds));
  rSurfaces.push_back(dSurface.get());

  // Polyhedron reference generator
  PolyhedronReferenceGenerator<1u, true> rGenerator;
  // A single proto axis clused in phi with 44 bins
  ProtoAxis pAxis(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                  -std::numbers::pi, std::numbers::pi, 44u);
  auto indexedRing =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          Experimental::IndexedSurfacesNavigation>(
          tContext, rSurfaces, rGenerator, pAxis, 0u, {rSurfaces.size() - 1u});
  // The displaying
  auto pIndexedRing = IndexedSurfacesConverter::convert(
      tContext, rSurfaces, indexedRing, drawOptions);
  auto pIndexRingView = View::xy(pIndexedRing, "RingDisc1DWithSupport");
  toFile({pIndexRingView}, pIndexRingView._id + ".svg");
}

BOOST_AUTO_TEST_CASE(RingDisc2D) {
  // Two rings to make a disc
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfacesR0 = cGeometry.surfacesRing(dStore, 6.4, 12.4, 18., 0.125, 0.,
                                            42., 0., 2., 22u);

  auto rSurfacesR1 = cGeometry.surfacesRing(dStore, 12.4, 20.4, 30., 0.125, 0.,
                                            80., 0., 2., 22u);

  decltype(rSurfacesR0) rSurfaces = rSurfacesR0;
  rSurfaces.insert(rSurfaces.end(), rSurfacesR1.begin(), rSurfacesR1.end());

  ProtoAxis pAxisR(AxisDirection::AxisR, AxisBoundaryType::Bound,
                   {24., 74., 110});
  ProtoAxis pAxisPhi(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                     -std::numbers::pi, std::numbers::pi, 44u);

  PolyhedronReferenceGenerator<1u, true> rGenerator;

  auto indexedRing =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          Experimental::IndexedSurfacesNavigation>(
          tContext, rSurfaces, rGenerator, pAxisR, 0u, pAxisPhi, 0u);

  // The displaying
  auto pIndexedRing = IndexedSurfacesConverter::convert(
      tContext, rSurfaces, indexedRing, drawOptions);
  auto pIndexRingView = View::xy(pIndexedRing, "RingDisc2D");

  toFile({pIndexRingView}, pIndexRingView._id + ".svg");
}

BOOST_AUTO_TEST_CASE(RingDisc2DFine) {
  // Three rings to make a disc
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfacesR0 = cGeometry.surfacesRing(dStore, 6.4, 12.4, 18., 0.125, 0.,
                                            42., 0., 2., 22u);

  auto rSurfacesR1 = cGeometry.surfacesRing(dStore, 12.4, 20.4, 30., 0.125, 0.,
                                            80., 0., 2., 22u);

  auto rSurfacesR2 = cGeometry.surfacesRing(dStore, 18.4, 28.4, 30., 0.125, 0.,
                                            122., 0., 2., 36u);

  decltype(rSurfacesR0) rSurfaces = rSurfacesR0;
  rSurfaces.insert(rSurfaces.end(), rSurfacesR1.begin(), rSurfacesR1.end());
  rSurfaces.insert(rSurfaces.end(), rSurfacesR2.begin(), rSurfacesR2.end());

  ProtoAxis pAxisR(AxisDirection::AxisR, AxisBoundaryType::Bound, 24., 152, 8u);
  ProtoAxis pAxisPhi(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                     -std::numbers::pi, std::numbers::pi, 88u);

  PolyhedronReferenceGenerator<1u, true> rGenerator;

  auto indexedRing =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          Experimental::IndexedSurfacesNavigation>(
          tContext, rSurfaces, rGenerator, pAxisR, 0u, pAxisPhi, 0u);

  // The displaying
  auto pIndexedRing = IndexedSurfacesConverter::convert(
      tContext, rSurfaces, indexedRing, drawOptions);
  auto pIndexRingView = View::xy(pIndexedRing, "RingDisc2DFine");

  toFile({pIndexRingView}, pIndexRingView._id + ".svg");
}

BOOST_AUTO_TEST_CASE(RingDisc2DFineExpanded) {
  // Three rings to make a disc
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfacesR0 = cGeometry.surfacesRing(dStore, 6.4, 12.4, 18., 0.125, 0.,
                                            42., 0., 2., 22u);

  auto rSurfacesR1 = cGeometry.surfacesRing(dStore, 12.4, 20.4, 30., 0.125, 0.,
                                            80., 0., 2., 22u);

  auto rSurfacesR2 = cGeometry.surfacesRing(dStore, 18.4, 28.4, 30., 0.125, 0.,
                                            122., 0., 2., 36u);

  decltype(rSurfacesR0) rSurfaces = rSurfacesR0;
  rSurfaces.insert(rSurfaces.end(), rSurfacesR1.begin(), rSurfacesR1.end());
  rSurfaces.insert(rSurfaces.end(), rSurfacesR2.begin(), rSurfacesR2.end());

  PolyhedronReferenceGenerator<1u, true> rGenerator;

  ProtoAxis pAxisR(AxisDirection::AxisR, AxisBoundaryType::Bound, 24., 152, 8u);
  ProtoAxis pAxisPhi(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                     -std::numbers::pi, std::numbers::pi, 88u);

  auto indexedRing =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          Experimental::IndexedSurfacesNavigation>(
          tContext, rSurfaces, rGenerator, pAxisR, 2u, pAxisPhi, 4u);

  // The displaying
  auto pIndexedRing = IndexedSurfacesConverter::convert(
      tContext, rSurfaces, indexedRing, drawOptions);
  auto pIndexRingView = View::xy(pIndexedRing, "RingDisc2DFineExpanded");

  toFile({pIndexRingView}, pIndexRingView._id + ".svg");
}

BOOST_AUTO_TEST_CASE(Cylinder2D) {
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto surfaces = cGeometry.surfacesCylinder(dStore, 8.4, 36., 0.15, 0.145,
                                             116., 3., 2., {52, 14});

  ProtoAxis pAxisZ(AxisDirection::AxisZ, AxisBoundaryType::Bound, -500., 500.,
                   28u);
  ProtoAxis pAxisPhi(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                     -std::numbers::pi, std::numbers::pi, 52u);
  PolyhedronReferenceGenerator<1u, true> rGenerator;

  auto indexedCylinder =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          Experimental::IndexedSurfacesNavigation>(
          tContext, surfaces, rGenerator, pAxisZ, 1u, pAxisPhi, 1u);

  // The displaying
  auto pIndexeCylinder = IndexedSurfacesConverter::convert(
      tContext, surfaces, indexedCylinder, drawOptions);
  auto pIndexCylinderView = View::zphi(pIndexeCylinder, "Cylinder");

  toFile({pIndexCylinderView}, pIndexCylinderView._id + ".svg");
}

BOOST_AUTO_TEST_SUITE_END()
