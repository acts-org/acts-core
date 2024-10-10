// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/DiscBounds.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "ActsFatras/Digitization/Segmentizer.hpp"

#include <cmath>
#include <fstream>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "DigitizationCsvOutput.hpp"
#include "PlanarSurfaceTestBeds.hpp"

namespace bdata = boost::unit_test::data;

namespace ActsFatras {

BOOST_AUTO_TEST_SUITE(Digitization)

BOOST_AUTO_TEST_CASE(SegmentizerCartesian) {
  Acts::GeometryContext geoCtx;

  auto rectangleBounds = std::make_shared<Acts::RectangleBounds>(1., 1.);
  auto planeSurface = Acts::Surface::makeShared<Acts::PlaneSurface>(
      Acts::Transform3::Identity(), rectangleBounds);

  // The segmentation
  Acts::BinUtility pixelated(20, -1., 1., Acts::open, Acts::BinningValue::binX);
  pixelated +=
      Acts::BinUtility(20, -1., 1., Acts::open, Acts::BinningValue::binY);

  Segmentizer cl;

  // Test: Normal hit into the surface
  Acts::Vector2 nPosition(0.37, 0.76);
  auto nSegments =
      cl.segments(geoCtx, *planeSurface, pixelated, {nPosition, nPosition});
  BOOST_CHECK_EQUAL(nSegments.size(), 1);
  BOOST_CHECK_EQUAL(nSegments[0].bin[0], 13);
  BOOST_CHECK_EQUAL(nSegments[0].bin[1], 17);

  // Test: Inclined hit into the surface - negative x direction
  Acts::Vector2 ixPositionS(0.37, 0.76);
  Acts::Vector2 ixPositionE(0.02, 0.73);
  auto ixSegments =
      cl.segments(geoCtx, *planeSurface, pixelated, {ixPositionS, ixPositionE});
  BOOST_CHECK_EQUAL(ixSegments.size(), 4);

  // Test: Inclined hit into the surface - positive y direction
  Acts::Vector2 iyPositionS(0.37, 0.76);
  Acts::Vector2 iyPositionE(0.39, 0.91);
  auto iySegments =
      cl.segments(geoCtx, *planeSurface, pixelated, {iyPositionS, iyPositionE});
  BOOST_CHECK_EQUAL(iySegments.size(), 3);

  // Test: Inclined hit into the surface - x/y direction
  Acts::Vector2 ixyPositionS(-0.27, 0.76);
  Acts::Vector2 ixyPositionE(-0.02, -0.73);
  auto ixySegments = cl.segments(geoCtx, *planeSurface, pixelated,
                                 {ixyPositionS, ixyPositionE});
  BOOST_CHECK_EQUAL(ixySegments.size(), 18);
}

BOOST_AUTO_TEST_CASE(SegmentizerPolarRadial) {
  Acts::GeometryContext geoCtx;

  auto radialBounds =
      std::make_shared<const Acts::RadialBounds>(5., 10., 0.25, 0.);
  auto radialDisc = Acts::Surface::makeShared<Acts::DiscSurface>(
      Acts::Transform3::Identity(), radialBounds);

  // The segmentation
  Acts::BinUtility strips(2, 5., 10., Acts::open, Acts::BinningValue::binR);
  strips += Acts::BinUtility(250, -0.25, 0.25, Acts::open,
                             Acts::BinningValue::binPhi);

  Segmentizer cl;

  // Test: Normal hit into the surface
  Acts::Vector2 nPosition(6.76, 0.5);
  auto nSegments =
      cl.segments(geoCtx, *radialDisc, strips, {nPosition, nPosition});
  BOOST_CHECK_EQUAL(nSegments.size(), 1);
  BOOST_CHECK_EQUAL(nSegments[0].bin[0], 0);
  BOOST_CHECK_EQUAL(nSegments[0].bin[1], 161);

  // Test: now opver more phi strips
  Acts::Vector2 sPositionS(6.76, 0.5);
  Acts::Vector2 sPositionE(7.03, -0.3);
  auto sSegment =
      cl.segments(geoCtx, *radialDisc, strips, {sPositionS, sPositionE});
  BOOST_CHECK_EQUAL(sSegment.size(), 59);

  // Test: jump over R boundary, but stay in phi bin
  sPositionS = Acts::Vector2(6.76, 0.);
  sPositionE = Acts::Vector2(7.83, 0.);
  sSegment = cl.segments(geoCtx, *radialDisc, strips, {sPositionS, sPositionE});
  BOOST_CHECK_EQUAL(sSegment.size(), 2);
}

/// Unit test for testing the Segmentizer
BOOST_DATA_TEST_CASE(
    RandomSegmentizerTest,
    bdata::random((
        bdata::engine = std::mt19937(), bdata::seed = 1,
        bdata::distribution = std::uniform_real_distribution<double>(0., 1.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 2,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(0., 1.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 3,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(0., 1.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 4,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(0., 1.))) ^
        bdata::xrange(25),
    startR0, startR1, endR0, endR1, index) {
  Acts::GeometryContext geoCtx;
  Segmentizer cl;

  // Test beds with random numbers generated inside
  PlanarSurfaceTestBeds pstd;
  auto testBeds = pstd(1.);

  DigitizationCsvOutput csvHelper;

  for (const auto& tb : testBeds) {
    const auto& name = std::get<0>(tb);
    const auto* surface = (std::get<1>(tb)).get();
    const auto& segmentation = std::get<2>(tb);
    const auto& randomizer = std::get<3>(tb);

    if (index == 0) {
      std::ofstream shape;
      std::ofstream grid;
      const auto centerXY = surface->center(geoCtx).segment<2>(0);
      // 0 - write the shape
      shape.open("Segmentizer" + name + "Borders.csv");
      if (surface->type() == Acts::Surface::Plane) {
        const auto* pBounds =
            static_cast<const Acts::PlanarBounds*>(&(surface->bounds()));
        csvHelper.writePolygon(shape, pBounds->vertices(1), -centerXY);
      } else if (surface->type() == Acts::Surface::Disc) {
        const auto* dBounds =
            static_cast<const Acts::DiscBounds*>(&(surface->bounds()));
        csvHelper.writePolygon(shape, dBounds->vertices(72), -centerXY);
      }
      // 1 - write the grid
      grid.open("Segmentizer" + name + "Grid.csv");
      if (segmentation.binningData()[0].binvalue == Acts::BinningValue::binX &&
          segmentation.binningData()[1].binvalue == Acts::BinningValue::binY) {
        double bxmin = segmentation.binningData()[0].min;
        double bxmax = segmentation.binningData()[0].max;
        double bymin = segmentation.binningData()[1].min;
        double bymax = segmentation.binningData()[1].max;
        const auto& xboundaries = segmentation.binningData()[0].boundaries();
        const auto& yboundaries = segmentation.binningData()[1].boundaries();
        for (const auto xval : xboundaries) {
          csvHelper.writeLine(grid, {xval, bymin}, {xval, bymax});
        }
        for (const auto yval : yboundaries) {
          csvHelper.writeLine(grid, {bxmin, yval}, {bxmax, yval});
        }
      } else if (segmentation.binningData()[0].binvalue ==
                     Acts::BinningValue::binR &&
                 segmentation.binningData()[1].binvalue ==
                     Acts::BinningValue::binPhi) {
        double brmin = segmentation.binningData()[0].min;
        double brmax = segmentation.binningData()[0].max;
        double bphimin = segmentation.binningData()[1].min;
        double bphimax = segmentation.binningData()[1].max;
        const auto& rboundaries = segmentation.binningData()[0].boundaries();
        const auto& phiboundaries = segmentation.binningData()[1].boundaries();
        for (const auto r : rboundaries) {
          csvHelper.writeArc(grid, r, bphimin, bphimax);
        }
        for (const auto phi : phiboundaries) {
          double cphi = std::cos(phi);
          double sphi = std::sin(phi);
          csvHelper.writeLine(grid, {brmin * cphi, brmin * sphi},
                              {brmax * cphi, brmax * sphi});
        }
      }
    }

    auto start = randomizer(startR0, startR1);
    auto end = randomizer(endR0, endR1);

    std::ofstream segments;
    segments.open("Segmentizer" + name + "Segments_n" + std::to_string(index) +
                  ".csv");

    std::ofstream cluster;
    cluster.open("Segmentizer" + name + "Cluster_n" + std::to_string(index) +
                 ".csv");

    /// Run the Segmentizer
    auto cSegments = cl.segments(geoCtx, *surface, segmentation, {start, end});

    for (const auto& cs : cSegments) {
      csvHelper.writeLine(segments, cs.path2D[0], cs.path2D[1]);
    }

    segments.close();
    cluster.close();
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace ActsFatras
