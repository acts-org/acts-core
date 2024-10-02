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
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/DiscBounds.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Result.hpp"
#include "ActsFatras/Digitization/PlanarSurfaceMask.hpp"

#include <array>
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

std::vector<std::array<std::ofstream, 3>> out;

BOOST_AUTO_TEST_SUITE(Digitization)

BOOST_AUTO_TEST_CASE(PlaneMaskRectangleBounds) {
  auto rectangleBounds = std::make_shared<Acts::RectangleBounds>(2., 3.5);
  auto planeSurface = Acts::Surface::makeShared<Acts::PlaneSurface>(
      Acts::Transform3::Identity(), rectangleBounds);

  ActsFatras::PlanarSurfaceMask psm;

  /// Case one : one outside
  std::array<Acts::Vector2, 2> segment = {Acts::Vector2(2.5, -4.5),
                                          Acts::Vector2(-1., -1.)};
  auto clipped = psm.apply(*planeSurface, segment).value();

  CHECK_CLOSE_ABS(clipped[1].x(), segment[1].x(), Acts::s_epsilon);
  CHECK_CLOSE_ABS(clipped[1].y(), segment[1].y(), Acts::s_epsilon);
  CHECK_CLOSE_ABS(clipped[0].x(), 1.5, Acts::s_epsilon);
  CHECK_CLOSE_ABS(clipped[0].y(), -3.5, Acts::s_epsilon);

  /// Case two : two outside
  segment = {Acts::Vector2(1., 4.), Acts::Vector2(3., 2.)};
  clipped = psm.apply(*planeSurface, segment).value();

  CHECK_CLOSE_ABS(clipped[1].x(), 2., Acts::s_epsilon);
  CHECK_CLOSE_ABS(clipped[1].y(), 3., Acts::s_epsilon);
  CHECK_CLOSE_ABS(clipped[0].x(), 1.5, Acts::s_epsilon);
  CHECK_CLOSE_ABS(clipped[0].y(), 3.5, Acts::s_epsilon);

  /// Case two : both inside (most likely case, untouched)
  segment = {Acts::Vector2(-1., 0.5), Acts::Vector2(0., 2.)};
  clipped = psm.apply(*planeSurface, segment).value();

  CHECK_CLOSE_ABS(clipped[0].x(), segment[0].x(), Acts::s_epsilon);
  CHECK_CLOSE_ABS(clipped[0].y(), segment[0].y(), Acts::s_epsilon);
  CHECK_CLOSE_ABS(clipped[1].x(), segment[1].x(), Acts::s_epsilon);
  CHECK_CLOSE_ABS(clipped[1].y(), segment[1].y(), Acts::s_epsilon);
}

BOOST_AUTO_TEST_CASE(DiscMaskRadialBounds) {
  auto discRadial =
      std::make_shared<Acts::RadialBounds>(2., 7.5, M_PI_4, M_PI_2);
  auto discSurface = Acts::Surface::makeShared<Acts::DiscSurface>(
      Acts::Transform3::Identity(), discRadial);

  ActsFatras::PlanarSurfaceMask psm;

  /// Case one : one outside R min
  std::array<Acts::Vector2, 2> segment = {Acts::Vector2(0.5, 1.8),
                                          Acts::Vector2(0.9, 6.)};
  auto clipped = psm.apply(*discSurface, segment).value();

  CHECK_CLOSE_ABS(clipped[1].x(), segment[1].x(), Acts::s_epsilon);
  CHECK_CLOSE_ABS(clipped[1].y(), segment[1].y(), Acts::s_epsilon);
  CHECK_CLOSE_ABS(Acts::VectorHelpers::perp(clipped[0]), 2.,
                  5 * Acts::s_epsilon);

  /// Case two : one outside R max
  segment = {Acts::Vector2(0.5, 2.8), Acts::Vector2(0.9, 8.5)};
  clipped = psm.apply(*discSurface, segment).value();

  CHECK_CLOSE_ABS(clipped[0].x(), segment[0].x(), Acts::s_epsilon);
  CHECK_CLOSE_ABS(clipped[0].y(), segment[0].y(), Acts::s_epsilon);
  CHECK_CLOSE_ABS(Acts::VectorHelpers::perp(clipped[1]), 7.5,
                  5 * Acts::s_epsilon);

  /// Case three : both outside R min / max
  segment = {Acts::Vector2(0.5, 1.8), Acts::Vector2(0.9, 8.5)};
  clipped = psm.apply(*discSurface, segment).value();
  CHECK_CLOSE_ABS(Acts::VectorHelpers::perp(clipped[0]), 2.,
                  5 * Acts::s_epsilon);
  CHECK_CLOSE_ABS(Acts::VectorHelpers::perp(clipped[1]), 7.5,
                  5 * Acts::s_epsilon);
  /// Case four: outside phi min
  segment = {Acts::Vector2(2.8, 2.5), Acts::Vector2(0., 3.5)};
  clipped = psm.apply(*discSurface, segment).value();
  CHECK_CLOSE_ABS(Acts::VectorHelpers::phi(clipped[0]), M_PI_4,
                  Acts::s_epsilon);

  /// Case five: outside phi max
  segment = {Acts::Vector2(0., 3.5), Acts::Vector2(-8., 5.)};
  clipped = psm.apply(*discSurface, segment).value();
  CHECK_CLOSE_ABS(Acts::VectorHelpers::phi(clipped[1]), M_PI_2 + M_PI_4,
                  Acts::s_epsilon);
}

std::vector<std::array<std::ofstream, 3>> segmentOutput;
int ntests = 100;

/// Unit test for testing the Surface mask
BOOST_DATA_TEST_CASE(
    RandomPlanarSurfaceMask,
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
        bdata::xrange(ntests),
    startR0, startR1, endR0, endR1, index) {
  Acts::GeometryContext geoCtx;

  ActsFatras::PlanarSurfaceMask psm;

  // Test beds with random numbers generated inside
  PlanarSurfaceTestBeds pstd;
  // Smearing 10 percent outside
  auto testBeds = pstd(1.1);

  DigitizationCsvOutput csvHelper;

  int itb = 0;
  for (const auto& tb : testBeds) {
    const auto& name = std::get<0>(tb);
    const auto* surface = (std::get<1>(tb)).get();
    const auto& randomizer = std::get<3>(tb);

    if (index == 0) {
      std::ofstream shape;
      const Acts::Vector2 centerXY = surface->center(geoCtx).segment<2>(0);

      // 0 - write the shape
      shape.open("PlanarSurfaceMask" + name + "Borders.csv");
      if (surface->type() == Acts::Surface::Plane) {
        const auto* pBounds =
            static_cast<const Acts::PlanarBounds*>(&(surface->bounds()));
        csvHelper.writePolygon(shape, pBounds->vertices(1), -centerXY);
      } else if (surface->type() == Acts::Surface::Disc) {
        const auto* dBounds =
            static_cast<const Acts::DiscBounds*>(&(surface->bounds()));
        csvHelper.writePolygon(shape, dBounds->vertices(72), -centerXY);
      }

      segmentOutput.push_back(std::array<std::ofstream, 3>());
      segmentOutput[itb][0].open("PlanarSurfaceMask" + name + "Inside.csv");
      segmentOutput[itb][1].open("PlanarSurfaceMask" + name + "Clipped.csv");
      segmentOutput[itb][2].open("PlanarSurfaceMask" + name + "Outside.csv");
    }

    auto start = randomizer(startR0, startR1);
    auto end = randomizer(endR0, endR1);

    std::array<Acts::Vector2, 2> segment = {start, end};
    auto clippedTest = psm.apply(*surface, segment);
    if (clippedTest.ok()) {
      auto clipped = clippedTest.value();
      if (segment == clipped) {
        csvHelper.writeLine(segmentOutput[itb][0], start, end);
      } else {
        csvHelper.writeLine(segmentOutput[itb][1], clipped[0], clipped[1]);
      }
    } else {
      csvHelper.writeLine(segmentOutput[itb][2], start, end);
    }
    ++itb;
  }

  // close the lines
  if (itb == ntests - 1) {
    segmentOutput[itb][0].close();
    segmentOutput[itb][1].close();
    segmentOutput[itb][2].close();
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace ActsFatras
