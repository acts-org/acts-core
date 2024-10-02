// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/detail/grid_helper.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace Acts::detail;

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(GridTests)

BOOST_AUTO_TEST_CASE(grid_test_1d_equidistant) {
  using Point = std::array<double, 1>;
  using indices = std::array<std::size_t, 1>;
  Axis a(0.0, 4.0, 4u);
  Grid g(Type<double>, std::move(a));

  // test general properties
  BOOST_CHECK_EQUAL(g.size(), 6u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(0), 4u);

  // global bin index
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-0.3}})), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-0.}})), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.}})), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.7}})), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1}})), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1.2}})), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2.}})), 3u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2.7}})), 3u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3.}})), 4u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3.9999}})), 4u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{4.}})), 5u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{4.98}})), 5u);

  // global bin index -> local bin indices
  BOOST_CHECK(g.localBinsFromGlobalBin(0) == indices({{0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(1) == indices({{1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(2) == indices({{2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(3) == indices({{3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(4) == indices({{4}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(5) == indices({{5}}));

  // local bin indices -> global bin index
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0}}), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1}}), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2}}), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3}}), 3u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{4}}), 4u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{5}}), 5u);

  BOOST_CHECK(g.localBinsFromGlobalBin(
                  g.globalBinFromPosition(Point({{2.7}}))) == indices({{3}}));

  // inside checks
  BOOST_CHECK(!g.isInside(Point({{-2.}})));
  BOOST_CHECK(g.isInside(Point({{0.}})));
  BOOST_CHECK(g.isInside(Point({{2.5}})));
  BOOST_CHECK(!g.isInside(Point({{4.}})));
  BOOST_CHECK(!g.isInside(Point({{6.}})));

  // test some bin centers
  CHECK_CLOSE_ABS(g.binCenter({{1}}), Point({{0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{2}}), Point({{1.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{3}}), Point({{2.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{4}}), Point({{3.5}}), 1e-6);

  // test some lower-left bin edges
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1}}), Point({{0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{2}}), Point({{1.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{3}}), Point({{2.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{4}}), Point({{3.}}), 1e-6);

  // test some upper right-bin edges
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1}}), Point({{1.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{2}}), Point({{2.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{3}}), Point({{3.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{4}}), Point({{4.}}), 1e-6);

  // initialize grid
  for (std::size_t bin = 0; bin < g.size(); ++bin) {
    g.at(bin) = bin;
  }

  // consistency of access
  const auto& point = Point({{0.7}});
  std::size_t globalBin = g.globalBinFromPosition(point);
  indices localBins = g.localBinsFromGlobalBin(globalBin);

  BOOST_CHECK_EQUAL(g.atPosition(point), g.at(globalBin));
  BOOST_CHECK_EQUAL(g.atPosition(point), g.atLocalBins(localBins));
}

BOOST_AUTO_TEST_CASE(grid_test_2d_equidistant) {
  using Point = std::array<double, 2>;
  using indices = std::array<std::size_t, 2>;
  Axis a(0.0, 4.0, 4u);
  Axis b(0.0, 3.0, 3u);
  Grid g(Type<double>, std::move(a), std::move(b));

  // test general properties
  BOOST_CHECK_EQUAL(g.size(), 30u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(0), 4u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(1), 3u);

  // global bin index
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-1, -1}})), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-1, 0}})), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-1, 1}})), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-1, 2}})), 3u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-1, 3}})), 4u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, -1}})), 5u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0}})), 6u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 1}})), 7u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 2}})), 8u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 3}})), 9u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, -1}})), 10u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0}})), 11u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 1}})), 12u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 2}})), 13u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 3}})), 14u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, -1}})), 15u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 0}})), 16u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 1}})), 17u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 2}})), 18u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 3}})), 19u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3, -1}})), 20u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3, 0}})), 21u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3, 1}})), 22u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3, 2}})), 23u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3, 3}})), 24u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{4, -1}})), 25u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{4, 0}})), 26u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{4, 1}})), 27u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{4, 2}})), 28u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{4, 3}})), 29u);

  // test some arbitrary points
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1.2, 0.3}})), 11u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2.2, 3.3}})), 19u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.9, 1.8}})), 7u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3.7, 3.1}})), 24u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1.4, 2.3}})), 13u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-3, 3}})), 4u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{8, 1}})), 27u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, -3}})), 10u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3, 11}})), 24u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-2, -3}})), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-2, 7}})), 04u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{12, -1}})), 25u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{12, 11}})), 29u);

  // global bin index -> local bin indices
  BOOST_CHECK(g.localBinsFromGlobalBin(0) == indices({{0, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(1) == indices({{0, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(2) == indices({{0, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(3) == indices({{0, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(4) == indices({{0, 4}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(5) == indices({{1, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(6) == indices({{1, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(7) == indices({{1, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(8) == indices({{1, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(9) == indices({{1, 4}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(10) == indices({{2, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(11) == indices({{2, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(12) == indices({{2, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(13) == indices({{2, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(14) == indices({{2, 4}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(15) == indices({{3, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(16) == indices({{3, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(17) == indices({{3, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(18) == indices({{3, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(19) == indices({{3, 4}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(20) == indices({{4, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(21) == indices({{4, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(22) == indices({{4, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(23) == indices({{4, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(24) == indices({{4, 4}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(25) == indices({{5, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(26) == indices({{5, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(27) == indices({{5, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(28) == indices({{5, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(29) == indices({{5, 4}}));

  // local bin indices -> global bin index
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 0}}), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 1}}), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 2}}), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 3}}), 3u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 4}}), 4u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 0}}), 5u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 1}}), 6u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 2}}), 7u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 3}}), 8u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 4}}), 9u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 0}}), 10u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 1}}), 11u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 2}}), 12u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 3}}), 13u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 4}}), 14u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 0}}), 15u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 1}}), 16u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 2}}), 17u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 3}}), 18u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 4}}), 19u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{4, 0}}), 20u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{4, 1}}), 21u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{4, 2}}), 22u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{4, 3}}), 23u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{4, 4}}), 24u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{5, 0}}), 25u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{5, 1}}), 26u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{5, 2}}), 27u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{5, 3}}), 28u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{5, 4}}), 29u);

  BOOST_CHECK(g.localBinsFromGlobalBin(g.globalBinFromPosition(
                  Point({{1.2, 0.7}}))) == indices({{2, 1}}));

  // inside checks
  BOOST_CHECK(!g.isInside(Point({{-2., -1}})));
  BOOST_CHECK(!g.isInside(Point({{-2., 1.}})));
  BOOST_CHECK(!g.isInside(Point({{-2., 5.}})));
  BOOST_CHECK(!g.isInside(Point({{1., -1.}})));
  BOOST_CHECK(!g.isInside(Point({{6., -1.}})));
  BOOST_CHECK(g.isInside(Point({{0.5, 1.3}})));
  BOOST_CHECK(!g.isInside(Point({{4., -1.}})));
  BOOST_CHECK(!g.isInside(Point({{4., 0.3}})));
  BOOST_CHECK(!g.isInside(Point({{4., 3.}})));
  BOOST_CHECK(!g.isInside(Point({{-1., 3.}})));
  BOOST_CHECK(!g.isInside(Point({{2., 3.}})));
  BOOST_CHECK(!g.isInside(Point({{5., 3.}})));

  // test some bin centers
  CHECK_CLOSE_ABS(g.binCenter({{1, 1}}), Point({{0.5, 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{2, 3}}), Point({{1.5, 2.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{3, 1}}), Point({{2.5, 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{4, 2}}), Point({{3.5, 1.5}}), 1e-6);

  // test some lower-left bin edges
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 1}}), Point({{0., 0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{2, 3}}), Point({{1., 2.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{3, 1}}), Point({{2., 0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{4, 2}}), Point({{3., 1.}}), 1e-6);

  // test some upper right-bin edges
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 1}}), Point({{1., 1.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{2, 3}}), Point({{2., 3.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{3, 1}}), Point({{3., 1.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{4, 2}}), Point({{4., 2.}}), 1e-6);

  // initialize grid
  for (std::size_t bin = 0; bin < g.size(); ++bin) {
    g.at(bin) = bin;
  }

  // consistency of access
  const auto& point = Point({{0.7, 1.3}});
  std::size_t globalBin = g.globalBinFromPosition(point);
  indices localBins = g.localBinsFromGlobalBin(globalBin);

  BOOST_CHECK_EQUAL(g.atPosition(point), g.at(globalBin));
  BOOST_CHECK_EQUAL(g.atPosition(point), g.atLocalBins(localBins));
}

BOOST_AUTO_TEST_CASE(grid_test_3d_equidistant) {
  using Point = std::array<double, 3>;
  using indices = std::array<std::size_t, 3>;
  Axis a(0.0, 2.0, 2u);
  Axis b(0.0, 3.0, 3u);
  Axis c(0.0, 2.0, 2u);
  Grid g(Type<double>, std::move(a), std::move(b), std::move(c));

  // test general properties
  BOOST_CHECK_EQUAL(g.size(), 80u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(0), 2u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(1), 3u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(2), 2u);

  // test grid points
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0, 0}})), 25u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0, 1}})), 26u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0, 2}})), 27u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 1, 0}})), 29u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 1, 1}})), 30u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 1, 2}})), 31u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 2, 0}})), 33u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 2, 1}})), 34u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 2, 2}})), 35u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 3, 0}})), 37u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 3, 1}})), 38u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 3, 2}})), 39u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0, 0}})), 45u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0, 1}})), 46u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0, 2}})), 47u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 1, 0}})), 49u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 1, 1}})), 50u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 1, 2}})), 51u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 2, 0}})), 53u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 2, 1}})), 54u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 2, 2}})), 55u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 3, 0}})), 57u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 3, 1}})), 58u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 3, 2}})), 59u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 0, 0}})), 65u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 0, 1}})), 66u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 0, 2}})), 67u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 1, 0}})), 69u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 1, 1}})), 70u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 1, 2}})), 71u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 2, 0}})), 73u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 2, 1}})), 74u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 2, 2}})), 75u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 3, 0}})), 77u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 3, 1}})), 78u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, 3, 2}})), 79u);

  // global bin index -> local bin indices
  BOOST_CHECK(g.localBinsFromGlobalBin(0) == indices({{0, 0, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(1) == indices({{0, 0, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(2) == indices({{0, 0, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(3) == indices({{0, 0, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(4) == indices({{0, 1, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(5) == indices({{0, 1, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(6) == indices({{0, 1, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(7) == indices({{0, 1, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(24) == indices({{1, 1, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(25) == indices({{1, 1, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(26) == indices({{1, 1, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(27) == indices({{1, 1, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(52) == indices({{2, 3, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(53) == indices({{2, 3, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(54) == indices({{2, 3, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(55) == indices({{2, 3, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(60) == indices({{3, 0, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(61) == indices({{3, 0, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(62) == indices({{3, 0, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(63) == indices({{3, 0, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(76) == indices({{3, 4, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(77) == indices({{3, 4, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(78) == indices({{3, 4, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(79) == indices({{3, 4, 3}}));

  // local bin indices -> global bin index
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 0, 0}}), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 0, 1}}), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 0, 2}}), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 0, 3}}), 3u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 1, 0}}), 4u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 1, 1}}), 5u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 1, 2}}), 6u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 1, 3}}), 7u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 1, 0}}), 24u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 1, 1}}), 25u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 1, 2}}), 26u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 1, 3}}), 27u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 3, 0}}), 52u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 3, 1}}), 53u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 3, 2}}), 54u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 3, 3}}), 55u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 0, 0}}), 60u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 0, 1}}), 61u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 0, 2}}), 62u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 0, 3}}), 63u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 4, 0}}), 76u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 4, 1}}), 77u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 4, 2}}), 78u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 4, 3}}), 79u);

  BOOST_CHECK(g.localBinsFromGlobalBin(g.globalBinFromPosition(
                  Point({{1.2, 0.7, 1.4}}))) == indices({{2, 1, 2}}));

  // inside checks
  BOOST_CHECK(!g.isInside(Point({{-2., -1, -2}})));
  BOOST_CHECK(!g.isInside(Point({{-2., 1., 0.}})));
  BOOST_CHECK(!g.isInside(Point({{-2., 5., -1}})));
  BOOST_CHECK(!g.isInside(Point({{1., -1., 1.}})));
  BOOST_CHECK(!g.isInside(Point({{6., -1., 4.}})));
  BOOST_CHECK(g.isInside(Point({{0.5, 1.3, 1.7}})));
  BOOST_CHECK(!g.isInside(Point({{2., -1., -0.4}})));
  BOOST_CHECK(!g.isInside(Point({{2., 0.3, 3.4}})));
  BOOST_CHECK(!g.isInside(Point({{2., 3., 0.8}})));
  BOOST_CHECK(!g.isInside(Point({{-1., 3., 5.}})));
  BOOST_CHECK(!g.isInside(Point({{2., 3., -1.}})));
  BOOST_CHECK(!g.isInside(Point({{5., 3., 0.5}})));

  // test some bin centers
  CHECK_CLOSE_ABS(g.binCenter({{1, 1, 1}}), Point({{0.5, 0.5, 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{2, 3, 2}}), Point({{1.5, 2.5, 1.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{1, 1, 2}}), Point({{0.5, 0.5, 1.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{2, 2, 1}}), Point({{1.5, 1.5, 0.5}}), 1e-6);

  // test some lower-left bin edges
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 1, 1}}), Point({{0., 0., 0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{2, 3, 2}}), Point({{1., 2., 1.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 1, 2}}), Point({{0., 0., 1.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{2, 2, 1}}), Point({{1., 1., 0.}}), 1e-6);

  // test some upper right-bin edges
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 1, 1}}), Point({{1., 1., 1.}}),
                  1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{2, 3, 2}}), Point({{2., 3., 2.}}),
                  1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 1, 2}}), Point({{1., 1., 2.}}),
                  1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{2, 2, 1}}), Point({{2., 2., 1.}}),
                  1e-6);

  // initialize grid
  for (std::size_t bin = 0; bin < g.size(); ++bin) {
    g.at(bin) = bin;
  }

  // consistency of access
  const auto& point = Point({{0.7, 2.3, 1.3}});
  std::size_t globalBin = g.globalBinFromPosition(point);
  indices localBins = g.localBinsFromGlobalBin(globalBin);

  BOOST_CHECK_EQUAL(g.atPosition(point), g.at(globalBin));
  BOOST_CHECK_EQUAL(g.atPosition(point), g.atLocalBins(localBins));
}

BOOST_AUTO_TEST_CASE(grid_test_1d_variable) {
  using Point = std::array<double, 1>;
  using indices = std::array<std::size_t, 1>;
  Axis a({0.0, 1.0, 4.0});
  Grid g(Type<double>, std::move(a));

  // test general properties
  BOOST_CHECK_EQUAL(g.size(), 4u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(0), 2u);

  // global bin index
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-0.3}})), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.}})), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.7}})), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1}})), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1.2}})), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2.7}})), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{4.}})), 3u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{4.98}})), 3u);

  // global bin index -> local bin indices
  BOOST_CHECK(g.localBinsFromGlobalBin(0) == indices({{0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(1) == indices({{1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(2) == indices({{2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(3) == indices({{3}}));

  // local bin indices -> global bin index
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0}}), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1}}), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2}}), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3}}), 3u);

  BOOST_CHECK(g.localBinsFromGlobalBin(
                  g.globalBinFromPosition(Point({{0.8}}))) == indices({{1}}));

  // inside checks
  BOOST_CHECK(!g.isInside(Point({{-2.}})));
  BOOST_CHECK(g.isInside(Point({{0.}})));
  BOOST_CHECK(g.isInside(Point({{2.5}})));
  BOOST_CHECK(!g.isInside(Point({{4.}})));
  BOOST_CHECK(!g.isInside(Point({{6.}})));

  // test some bin centers
  CHECK_CLOSE_ABS(g.binCenter({{1}}), Point({{0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{2}}), Point({{2.5}}), 1e-6);

  // test some lower-left bin edges
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1}}), Point({{0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{2}}), Point({{1.}}), 1e-6);

  // test some upper right-bin edges
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1}}), Point({{1.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{2}}), Point({{4.}}), 1e-6);

  // initialize grid
  for (std::size_t bin = 0; bin < g.size(); ++bin) {
    g.at(bin) = bin;
  }

  // consistency of access
  const auto& point = Point({{0.7}});
  std::size_t globalBin = g.globalBinFromPosition(point);
  indices localBins = g.localBinsFromGlobalBin(globalBin);

  BOOST_CHECK_EQUAL(g.atPosition(point), g.at(globalBin));
  BOOST_CHECK_EQUAL(g.atPosition(point), g.atLocalBins(localBins));
}

BOOST_AUTO_TEST_CASE(grid_test_2d_variable) {
  using Point = std::array<double, 2>;
  using indices = std::array<std::size_t, 2>;
  Axis a({0.0, 0.5, 3.0});
  Axis b({0.0, 1.0, 4.0});
  Grid g(Type<double>, std::move(a), std::move(b));

  // test general properties
  BOOST_CHECK_EQUAL(g.size(), 16u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(0), 2u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(1), 2u);

  // test grid points
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0}})), 5u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 1}})), 6u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 4}})), 7u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.5, 0}})), 9u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.5, 1}})), 10u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.5, 4}})), 11u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3, 0}})), 13u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3, 1}})), 14u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3, 4}})), 15u);

  // test some arbitrary points
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.3, 1.2}})), 6u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3.3, 2.2}})), 14u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1.8, 0.9}})), 9u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{3.1, 0.7}})), 13u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2.3, 1.4}})), 10u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{2, -3}})), 8u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 8}})), 11u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-3, 1}})), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{11, 3}})), 14u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-3, -2}})), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{7, -2}})), 12u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-1, 12}})), 3u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{11, 12}})), 15u);

  // global bin index -> local bin indices
  BOOST_CHECK(g.localBinsFromGlobalBin(0) == indices({{0, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(1) == indices({{0, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(2) == indices({{0, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(3) == indices({{0, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(4) == indices({{1, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(5) == indices({{1, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(6) == indices({{1, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(7) == indices({{1, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(8) == indices({{2, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(9) == indices({{2, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(10) == indices({{2, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(11) == indices({{2, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(12) == indices({{3, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(13) == indices({{3, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(14) == indices({{3, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(15) == indices({{3, 3}}));

  // local bin indices -> global bin index
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 0}}), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 1}}), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 2}}), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 3}}), 3u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 0}}), 4u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 1}}), 5u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 2}}), 6u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 3}}), 7u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 0}}), 8u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 1}}), 9u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 2}}), 10u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 3}}), 11u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 0}}), 12u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 1}}), 13u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 2}}), 14u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 3}}), 15u);

  BOOST_CHECK(g.localBinsFromGlobalBin(g.globalBinFromPosition(
                  Point({{3.2, 1.8}}))) == indices({{3, 2}}));

  // inside checks
  BOOST_CHECK(!g.isInside(Point({{-2., -1}})));
  BOOST_CHECK(!g.isInside(Point({{-2., 1.}})));
  BOOST_CHECK(!g.isInside(Point({{-2., 5.}})));
  BOOST_CHECK(!g.isInside(Point({{1., -1.}})));
  BOOST_CHECK(!g.isInside(Point({{6., -1.}})));
  BOOST_CHECK(g.isInside(Point({{0.5, 1.3}})));
  BOOST_CHECK(!g.isInside(Point({{3., -1.}})));
  BOOST_CHECK(!g.isInside(Point({{3., 0.3}})));
  BOOST_CHECK(!g.isInside(Point({{3., 4.}})));
  BOOST_CHECK(!g.isInside(Point({{-1., 4.}})));
  BOOST_CHECK(!g.isInside(Point({{2., 4.}})));
  BOOST_CHECK(!g.isInside(Point({{5., 4.}})));

  // test some bin centers
  CHECK_CLOSE_ABS(g.binCenter({{1, 1}}), Point({{0.25, 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{2, 1}}), Point({{1.75, 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{1, 2}}), Point({{0.25, 2.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{2, 2}}), Point({{1.75, 2.5}}), 1e-6);

  // test some lower-left bin edges
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 1}}), Point({{0., 0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{2, 1}}), Point({{0.5, 0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 2}}), Point({{0., 1.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{2, 2}}), Point({{0.5, 1.}}), 1e-6);

  // test some upper right-bin edges
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 1}}), Point({{0.5, 1.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{2, 1}}), Point({{3., 1.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 2}}), Point({{0.5, 4.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{2, 2}}), Point({{3., 4.}}), 1e-6);

  // initialize grid
  for (std::size_t bin = 0; bin < g.size(); ++bin) {
    g.at(bin) = bin;
  }

  // consistency of access
  const auto& point = Point({{0.7, 1.3}});
  std::size_t globalBin = g.globalBinFromPosition(point);
  indices localBins = g.localBinsFromGlobalBin(globalBin);

  BOOST_CHECK_EQUAL(g.atPosition(point), g.at(globalBin));
  BOOST_CHECK_EQUAL(g.atPosition(point), g.atLocalBins(localBins));
}

BOOST_AUTO_TEST_CASE(grid_test_3d_variable) {
  using Point = std::array<double, 3>;
  using indices = std::array<std::size_t, 3>;
  Axis a({0.0, 1.0});
  Axis b({0.0, 0.5, 3.0});
  Axis c({0.0, 0.5, 3.0, 3.3});
  Grid g(Type<double>, std::move(a), std::move(b), std::move(c));

  // test general properties
  BOOST_CHECK_EQUAL(g.size(), 60u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(0), 1u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(1), 2u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(2), 3u);

  // test grid points
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0, 0}})), 26u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0, 0}})), 46u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0.5, 0}})), 31u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0.5, 0}})), 51u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 3, 0}})), 36u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 3, 0}})), 56u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0, 0.5}})), 27u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0, 0.5}})), 47u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0.5, 0.5}})), 32u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0.5, 0.5}})), 52u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 3, 0.5}})), 37u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 3, 0.5}})), 57u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0, 3}})), 28u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0, 3}})), 48u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0.5, 3}})), 33u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0.5, 3}})), 53u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 3, 3}})), 38u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 3, 3}})), 58u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0, 3.3}})), 29u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0, 3.3}})), 49u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0.5, 3.3}})), 34u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0.5, 3.3}})), 54u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 3, 3.3}})), 39u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 3, 3.3}})), 59u);

  // global bin index -> local bin indices
  BOOST_CHECK(g.localBinsFromGlobalBin(0) == indices({{0, 0, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(1) == indices({{0, 0, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(2) == indices({{0, 0, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(3) == indices({{0, 0, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(4) == indices({{0, 0, 4}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(5) == indices({{0, 1, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(21) == indices({{1, 0, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(22) == indices({{1, 0, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(23) == indices({{1, 0, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(24) == indices({{1, 0, 4}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(25) == indices({{1, 1, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(26) == indices({{1, 1, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(57) == indices({{2, 3, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(58) == indices({{2, 3, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(59) == indices({{2, 3, 4}}));

  // local bin indices -> global bin index
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 0, 0}}), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 0, 0}}), 20u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 0, 0}}), 40u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 1, 0}}), 5u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 1, 0}}), 25u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 1, 0}}), 45u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 3, 1}}), 16u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 3, 1}}), 36u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 3, 1}}), 56u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 0, 2}}), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 0, 2}}), 22u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 0, 2}}), 42u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 3, 4}}), 19u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 3, 4}}), 39u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 3, 4}}), 59u);

  BOOST_CHECK(g.localBinsFromGlobalBin(g.globalBinFromPosition(
                  Point({{1.8, 0.7, 3.2}}))) == indices({{2, 2, 3}}));

  // inside checks
  BOOST_CHECK(!g.isInside(Point({{-2., -1, -2}})));
  BOOST_CHECK(!g.isInside(Point({{-2., 1., 0.}})));
  BOOST_CHECK(!g.isInside(Point({{-2., 5., -1}})));
  BOOST_CHECK(!g.isInside(Point({{1., -1., 1.}})));
  BOOST_CHECK(!g.isInside(Point({{6., -1., 4.}})));
  BOOST_CHECK(g.isInside(Point({{0.5, 1.3, 1.7}})));
  BOOST_CHECK(!g.isInside(Point({{1., -1., -0.4}})));
  BOOST_CHECK(!g.isInside(Point({{1., 0.3, 3.4}})));
  BOOST_CHECK(!g.isInside(Point({{1., 3., 0.8}})));
  BOOST_CHECK(!g.isInside(Point({{-1., 3., 5.}})));
  BOOST_CHECK(!g.isInside(Point({{2., 3., -1.}})));
  BOOST_CHECK(!g.isInside(Point({{5., 3., 0.5}})));

  // test some bin centers
  CHECK_CLOSE_ABS(g.binCenter({{1, 1, 1}}), Point({{0.5, 0.25, 0.25}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{1, 1, 2}}), Point({{0.5, 0.25, 1.75}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{1, 1, 3}}), Point({{0.5, 0.25, 3.15}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{1, 2, 1}}), Point({{0.5, 1.75, 0.25}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{1, 2, 2}}), Point({{0.5, 1.75, 1.75}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{1, 2, 3}}), Point({{0.5, 1.75, 3.15}}), 1e-6);

  // test some lower-left bin edges
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 1, 1}}), Point({{0., 0., 0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 1, 2}}), Point({{0., 0., 0.5}}),
                  1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 1, 3}}), Point({{0., 0., 3.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 2, 1}}), Point({{0., 0.5, 0.}}),
                  1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 2, 2}}), Point({{0., 0.5, 0.5}}),
                  1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 2, 3}}), Point({{0., 0.5, 3.}}),
                  1e-6);

  // test some upper right-bin edges
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 1, 1}}), Point({{1., 0.5, 0.5}}),
                  1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 1, 2}}), Point({{1., 0.5, 3.}}),
                  1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 1, 3}}), Point({{1., 0.5, 3.3}}),
                  1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 2, 1}}), Point({{1., 3., 0.5}}),
                  1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 2, 2}}), Point({{1., 3., 3.}}),
                  1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 2, 3}}), Point({{1., 3., 3.3}}),
                  1e-6);

  // initialize grid
  for (std::size_t bin = 0; bin < g.size(); ++bin) {
    g.at(bin) = bin;
  }

  // consistency of access
  const auto& point = Point({{0.7, 1.3, 3.7}});
  std::size_t globalBin = g.globalBinFromPosition(point);
  indices localBins = g.localBinsFromGlobalBin(globalBin);

  BOOST_CHECK_EQUAL(g.atPosition(point), g.at(globalBin));
  BOOST_CHECK_EQUAL(g.atPosition(point), g.atLocalBins(localBins));
}

BOOST_AUTO_TEST_CASE(grid_test_2d_mixed) {
  using Point = std::array<double, 2>;
  using indices = std::array<std::size_t, 2>;
  Axis a(0.0, 1.0, 4u);
  Axis b({0.0, 0.5, 3.0});
  Grid g(Type<double>, std::move(a), std::move(b));

  // test general properties
  BOOST_CHECK_EQUAL(g.size(), 24u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(0), 4u);
  BOOST_CHECK_EQUAL(g.numLocalBins().at(1), 2u);

  // test grid points
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0}})), 5u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.25, 0}})), 9u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.5, 0}})), 13u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.75, 0}})), 17u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0}})), 21u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 0.5}})), 6u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.25, 0.5}})), 10u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.5, 0.5}})), 14u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.75, 0.5}})), 18u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 0.5}})), 22u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0, 3}})), 7u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.25, 3}})), 11u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.5, 3}})), 15u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.75, 3}})), 19u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1, 3}})), 23u);

  // test some arbitrary points
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{1.2, 0.3}})), 21u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.2, 1.3}})), 6u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.9, 1.8}})), 18u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.7, 2.1}})), 14u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.4, 0.3}})), 9u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-3, 2}})), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{8, 1}})), 22u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.1, -3}})), 4u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{0.8, 11}})), 19u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-2, -3}})), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{-2, 7}})), 3u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{12, -1}})), 20u);
  BOOST_CHECK_EQUAL(g.globalBinFromPosition(Point({{12, 11}})), 23u);

  // global bin index -> local bin indices
  using indices = std::array<std::size_t, 2>;
  BOOST_CHECK(g.localBinsFromGlobalBin(0) == indices({{0, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(1) == indices({{0, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(2) == indices({{0, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(3) == indices({{0, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(4) == indices({{1, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(5) == indices({{1, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(6) == indices({{1, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(7) == indices({{1, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(8) == indices({{2, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(9) == indices({{2, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(10) == indices({{2, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(11) == indices({{2, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(12) == indices({{3, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(13) == indices({{3, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(14) == indices({{3, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(15) == indices({{3, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(16) == indices({{4, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(17) == indices({{4, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(18) == indices({{4, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(19) == indices({{4, 3}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(20) == indices({{5, 0}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(21) == indices({{5, 1}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(22) == indices({{5, 2}}));
  BOOST_CHECK(g.localBinsFromGlobalBin(23) == indices({{5, 3}}));

  // local bin indices -> global bin index
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 0}}), 0u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 1}}), 1u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 2}}), 2u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{0, 3}}), 3u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 0}}), 4u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 1}}), 5u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 2}}), 6u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{1, 3}}), 7u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 0}}), 8u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 1}}), 9u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 2}}), 10u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{2, 3}}), 11u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 0}}), 12u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 1}}), 13u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 2}}), 14u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{3, 3}}), 15u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{4, 0}}), 16u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{4, 1}}), 17u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{4, 2}}), 18u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{4, 3}}), 19u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{5, 0}}), 20u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{5, 1}}), 21u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{5, 2}}), 22u);
  BOOST_CHECK_EQUAL(g.globalBinFromLocalBins({{5, 3}}), 23u);

  BOOST_CHECK(g.localBinsFromGlobalBin(g.globalBinFromPosition(
                  Point({{1.1, 1.7}}))) == indices({{5, 2}}));

  // inside checks
  BOOST_CHECK(!g.isInside(Point({{-2., -1}})));
  BOOST_CHECK(!g.isInside(Point({{-2., 1.}})));
  BOOST_CHECK(!g.isInside(Point({{-2., 5.}})));
  BOOST_CHECK(!g.isInside(Point({{0.1, -1.}})));
  BOOST_CHECK(!g.isInside(Point({{6., -1.}})));
  BOOST_CHECK(g.isInside(Point({{0.5, 1.3}})));
  BOOST_CHECK(!g.isInside(Point({{1., -1.}})));
  BOOST_CHECK(!g.isInside(Point({{1., 0.3}})));
  BOOST_CHECK(!g.isInside(Point({{1., 3.}})));
  BOOST_CHECK(!g.isInside(Point({{-1., 3.}})));
  BOOST_CHECK(!g.isInside(Point({{0.2, 3.}})));
  BOOST_CHECK(!g.isInside(Point({{5., 3.}})));

  // test some bin centers
  CHECK_CLOSE_ABS(g.binCenter({{1, 1}}), Point({{0.125, 0.25}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{1, 2}}), Point({{0.125, 1.75}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{2, 1}}), Point({{0.375, 0.25}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{2, 2}}), Point({{0.375, 1.75}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{3, 1}}), Point({{0.625, 0.25}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{3, 2}}), Point({{0.625, 1.75}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{4, 1}}), Point({{0.875, 0.25}}), 1e-6);
  CHECK_CLOSE_ABS(g.binCenter({{4, 2}}), Point({{0.875, 1.75}}), 1e-6);

  // test some lower-left bin edges
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 1}}), Point({{0., 0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{1, 2}}), Point({{0., 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{2, 1}}), Point({{0.25, 0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{2, 2}}), Point({{0.25, 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{3, 1}}), Point({{0.5, 0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{3, 2}}), Point({{0.5, 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{4, 1}}), Point({{0.75, 0.}}), 1e-6);
  CHECK_CLOSE_ABS(g.lowerLeftBinEdge({{4, 2}}), Point({{0.75, 0.5}}), 1e-6);

  // test some upper-right bin edges
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 1}}), Point({{0.25, 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{1, 2}}), Point({{0.25, 3.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{2, 1}}), Point({{0.5, 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{2, 2}}), Point({{0.5, 3.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{3, 1}}), Point({{0.75, 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{3, 2}}), Point({{0.75, 3.}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{4, 1}}), Point({{1., 0.5}}), 1e-6);
  CHECK_CLOSE_ABS(g.upperRightBinEdge({{4, 2}}), Point({{1., 3.}}), 1e-6);

  // initialize grid
  for (std::size_t bin = 0; bin < g.size(); ++bin) {
    g.at(bin) = bin;
  }

  // consistency of access
  const auto& point = Point({{1.3, 3.7}});
  std::size_t globalBin = g.globalBinFromPosition(point);
  indices localBins = g.localBinsFromGlobalBin(globalBin);

  BOOST_CHECK_EQUAL(g.atPosition(point), g.at(globalBin));
  BOOST_CHECK_EQUAL(g.atPosition(point), g.atLocalBins(localBins));
}

BOOST_AUTO_TEST_CASE(grid_test_2d_mixed_at) {
  Axis a(0.0, 6.0, 4u);
  Axis b({0.0, 1.5, 3.0});
  Grid g(Type<double>, std::move(a), std::move(b));

  // initialize the grid
  using Point = std::array<double, 2>;
  g.atPosition(Point({{0, 0}})) = 0.;
  g.atPosition(Point({{1.5, 0}})) = 1.;
  g.atPosition(Point({{3, 0}})) = 2.;
  g.atPosition(Point({{4.5, 0}})) = 3.;
  g.atPosition(Point({{6, 0}})) = 4.;
  g.atPosition(Point({{0, 1.5}})) = 5.;
  g.atPosition(Point({{1.5, 1.5}})) = 6.;
  g.atPosition(Point({{3, 1.5}})) = 7.;
  g.atPosition(Point({{4.5, 1.5}})) = 8.;
  g.atPosition(Point({{6, 1.5}})) = 9.;
  g.atPosition(Point({{0, 3}})) = 10.;
  g.atPosition(Point({{1.5, 3}})) = 11.;
  g.atPosition(Point({{3, 3}})) = 12.;
  g.atPosition(Point({{4.5, 3}})) = 13.;
  g.atPosition(Point({{6, 3}})) = 14.;

  // test general properties
  BOOST_CHECK_EQUAL(g.size(), 24u);

  // test some arbitrary points
  BOOST_CHECK_EQUAL(g.atPosition(Point({{1.2, 0.3}})), 0.);
  BOOST_CHECK_EQUAL(g.atPosition(Point({{2.2, 1.3}})), 1.);
  BOOST_CHECK_EQUAL(g.atPosition(Point({{4.9, 1.8}})), 8.);
  BOOST_CHECK_EQUAL(g.atPosition(Point({{3.7, 2.1}})), 7.);
  BOOST_CHECK_EQUAL(g.atPosition(Point({{0.4, 2.3}})), 5.);
}

BOOST_AUTO_TEST_CASE(grid_interpolation) {
  using Point = std::array<double, 3>;
  Axis a(1.0, 3.0, 2u);
  Axis b(1.0, 5.0, 2u);
  Axis c(1.0, 7.0, 2u);
  Grid g(Type<double>, std::move(a), std::move(b), std::move(c));

  g.atPosition(Point({{1., 1., 1.}})) = 10.;
  g.atPosition(Point({{2., 1., 1.}})) = 20.;
  g.atPosition(Point({{1., 3., 1.}})) = 30.;
  g.atPosition(Point({{2., 3., 1.}})) = 40.;
  g.atPosition(Point({{1., 1., 4.}})) = 50.;
  g.atPosition(Point({{2., 1., 4.}})) = 60.;
  g.atPosition(Point({{1., 3., 4.}})) = 70.;
  g.atPosition(Point({{2., 3., 4.}})) = 80.;

  CHECK_CLOSE_REL(g.interpolate(Point({{1., 1., 1.}})), 10., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{2., 1., 1.}})), 20., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1., 3., 1.}})), 30., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{2., 3., 1.}})), 40., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1., 1., 4.}})), 50., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{2., 1., 4.}})), 60., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1., 3., 4.}})), 70., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{2., 3., 4.}})), 80., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1.5, 1., 1.}})), 15., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1.5, 3., 1.}})), 35., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1., 2., 1.}})), 20., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{2., 2., 1.}})), 30., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1.5, 1., 4.}})), 55., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1.5, 3., 4.}})), 75., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1., 2., 4.}})), 60., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{2., 2., 4.}})), 70., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1., 1., 2.5}})), 30., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1., 3., 2.5}})), 50., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{2., 1., 2.5}})), 40., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{2., 3., 2.5}})), 60., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1.5, 2., 2.5}})), 360. / 8, 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{1.3, 2.1, 1.6}})), 32., 1e-6);
  CHECK_CLOSE_REL(g.interpolate(Point({{2., 3., 4.}})), 80., 1e-6);
}

BOOST_AUTO_TEST_CASE(neighborhood) {
  using bins_t = std::vector<std::size_t>;

  Axis a(0.0, 1.0, 10u);
  Axis b(0.0, 1.0, 10u);
  Axis c(0.0, 1.0, 10u);
  Grid g1(Type<double>, Axis{a});
  Grid g2(Type<double>, Axis{a}, Axis{b});
  Grid g3(Type<double>, std::move(a), std::move(b), std::move(c));

  // clang-format off
    // 1D case
    BOOST_CHECK(g1.neighborHoodIndices({{0}}, 1).collectVector()
                == bins_t({0, 1}));
    BOOST_CHECK(g1.neighborHoodIndices({{0}}, 2).collectVector()
                == bins_t({0, 1, 2}));
    BOOST_CHECK(g1.neighborHoodIndices({{1}}, 1).collectVector()
                == bins_t({0, 1, 2}));
    BOOST_CHECK(g1.neighborHoodIndices({{1}}, 3).collectVector()
                == bins_t({0, 1, 2, 3, 4}));
    BOOST_CHECK(g1.neighborHoodIndices({{4}}, 2).collectVector()
                == bins_t({2, 3, 4, 5, 6}));
    BOOST_CHECK(g1.neighborHoodIndices({{9}}, 2).collectVector()
                == bins_t({7, 8, 9, 10, 11}));
    BOOST_CHECK(g1.neighborHoodIndices({{10}}, 2).collectVector()
                == bins_t({8, 9, 10, 11}));
    BOOST_CHECK(g1.neighborHoodIndices({{11}}, 2).collectVector()
                == bins_t({9, 10, 11}));

    // 2D case
    BOOST_CHECK(g2.neighborHoodIndices({{0, 0}}, 1).collectVector()
                == bins_t({0, 1, 12, 13}));
    BOOST_CHECK(g2.neighborHoodIndices({{0, 1}}, 1).collectVector()
                == bins_t({0, 1, 2, 12, 13, 14}));
    BOOST_CHECK(g2.neighborHoodIndices({{1, 0}}, 1).collectVector()
                == bins_t({0, 1, 12, 13, 24, 25}));
    BOOST_CHECK(g2.neighborHoodIndices({{1, 1}}, 1).collectVector()
                == bins_t({0, 1, 2, 12, 13, 14, 24, 25, 26}));
    BOOST_CHECK(g2.neighborHoodIndices({{5, 5}}, 1).collectVector()
                == bins_t({52, 53, 54, 64, 65, 66, 76, 77, 78}));
    BOOST_CHECK(g2.neighborHoodIndices({{9, 10}}, 2).collectVector()
                == bins_t({92, 93, 94, 95, 104, 105, 106, 107, 116, 117, 118,
                           119, 128, 129, 130, 131, 140, 141, 142, 143}));

    // 3D case
    BOOST_CHECK(g3.neighborHoodIndices({{0, 0, 0}}, 1).collectVector()
                == bins_t({0, 1, 12, 13, 144, 145, 156, 157}));
    BOOST_CHECK(g3.neighborHoodIndices({{0, 0, 1}}, 1).collectVector()
                == bins_t({0, 1, 2, 12, 13, 14, 144, 145, 146, 156, 157, 158}));
    BOOST_CHECK(g3.neighborHoodIndices({{0, 1, 0}}, 1).collectVector()
                == bins_t({0, 1, 12, 13, 24, 25, 144, 145, 156, 157, 168, 169}));
    BOOST_CHECK(g3.neighborHoodIndices({{1, 0, 0}}, 1).collectVector()
                == bins_t({0, 1, 12, 13, 144, 145, 156, 157, 288, 289, 300, 301}));
    BOOST_CHECK(g3.neighborHoodIndices({{0, 1, 1}}, 1).collectVector()
                == bins_t({0, 1, 2, 12, 13, 14, 24, 25, 26, 144, 145, 146,
                           156, 157, 158, 168, 169, 170}));
    BOOST_CHECK(g3.neighborHoodIndices({{1, 1, 1}}, 1).collectVector()
                == bins_t({0, 1, 2, 12, 13, 14, 24, 25, 26, 144, 145, 146,
                           156, 157, 158, 168, 169, 170, 288, 289, 290, 300,
                           301, 302, 312, 313, 314}));
    BOOST_CHECK(g3.neighborHoodIndices({{11, 10, 9}}, 1).collectVector()
                == bins_t({1556, 1557, 1558, 1568, 1569, 1570, 1580, 1581,
                           1582, 1700, 1701, 1702, 1712, 1713, 1714, 1724,
                           1725, 1726}));

    // Neighbors array
    std::array<std::pair<int,int>,1> a1;
    a1.at(0) = std::make_pair<int,int>(-1,1);
    BOOST_CHECK(g1.neighborHoodIndices({{0}}, a1).collectVector()
                == bins_t({0,1}));
    BOOST_CHECK(g1.neighborHoodIndices({{2}}, a1).collectVector()
                == bins_t({1,2,3}));

    a1.at(0) = std::make_pair<int,int>(2,3);
    BOOST_CHECK(g1.neighborHoodIndices({{2}}, a1).collectVector()
                == bins_t({4,5}));

    a1.at(0) = std::make_pair<int,int>(-2,-1);
    BOOST_CHECK(g1.neighborHoodIndices({{2}}, a1).collectVector()
                == bins_t({0,1}));

    a1.at(0) = std::make_pair<int,int>(-3,-1);
    BOOST_CHECK(g1.neighborHoodIndices({{2}}, a1).collectVector()
                == bins_t({0,1}));
  // clang-format on

  Axis d(AxisClosed, 0.0, 1.0, 10u);

  Grid g1Cl(Type<double>, std::move(d));
  BOOST_CHECK(g1Cl.neighborHoodIndices({{0}}, 1).collectVector() ==
              bins_t({}));  // underflow, makes no sense
  BOOST_CHECK(g1Cl.neighborHoodIndices({{11}}, 1).collectVector() ==
              bins_t({}));  // overflow, makes no sense
  BOOST_CHECK(g1Cl.neighborHoodIndices({{1}}, 1).collectVector() ==
              bins_t({10, 1, 2}));  // overflow, makes no sense
  BOOST_CHECK(g1Cl.neighborHoodIndices({{5}}, 1).collectVector() ==
              bins_t({4, 5, 6}));  // overflow, makes no sense

  Axis f(AxisClosed, 0.0, 1.0, 5u);
  Axis e(AxisClosed, 0.0, 1.0, 5u);
  Grid g2Cl(Type<double>, std::move(e), std::move(f));
  BOOST_CHECK(g2Cl.neighborHoodIndices({{3, 3}}, 1).collectVector() ==
              bins_t({16, 17, 18, 23, 24, 25, 30, 31, 32}));
  BOOST_CHECK(g2Cl.neighborHoodIndices({{1, 1}}, 1).collectVector() ==
              bins_t({40, 36, 37, 12, 8, 9, 19, 15, 16}));
  BOOST_CHECK(g2Cl.neighborHoodIndices({{1, 5}}, 1).collectVector() ==
              bins_t({39, 40, 36, 11, 12, 8, 18, 19, 15}));
  BOOST_CHECK(g2Cl.neighborHoodIndices({{5, 1}}, 1).collectVector() ==
              bins_t({33, 29, 30, 40, 36, 37, 12, 8, 9}));
  BOOST_CHECK(g2Cl.neighborHoodIndices({{5, 5}}, 1).collectVector() ==
              bins_t({32, 33, 29, 39, 40, 36, 11, 12, 8}));

  BOOST_CHECK(g2Cl.neighborHoodIndices({{3, 3}}, 2).collectVector() ==
              bins_t({8,  9,  10, 11, 12, 15, 16, 17, 18, 19, 22, 23, 24,
                      25, 26, 29, 30, 31, 32, 33, 36, 37, 38, 39, 40}));
  BOOST_CHECK(g2Cl.neighborHoodIndices({{1, 1}}, 2).collectVector() ==
              bins_t({32, 33, 29, 30, 31, 39, 40, 36, 37, 38, 11, 12, 8,
                      9,  10, 18, 19, 15, 16, 17, 25, 26, 22, 23, 24}));
  BOOST_CHECK(g2Cl.neighborHoodIndices({{1, 5}}, 2).collectVector() ==
              bins_t({31, 32, 33, 29, 30, 38, 39, 40, 36, 37, 10, 11, 12,
                      8,  9,  17, 18, 19, 15, 16, 24, 25, 26, 22, 23}));
  BOOST_CHECK(g2Cl.neighborHoodIndices({{5, 1}}, 2).collectVector() ==
              bins_t({25, 26, 22, 23, 24, 32, 33, 29, 30, 31, 39, 40, 36,
                      37, 38, 11, 12, 8,  9,  10, 18, 19, 15, 16, 17}));
  BOOST_CHECK(g2Cl.neighborHoodIndices({{5, 5}}, 2).collectVector() ==
              bins_t({24, 25, 26, 22, 23, 31, 32, 33, 29, 30, 38, 39, 40,
                      36, 37, 10, 11, 12, 8,  9,  17, 18, 19, 15, 16}));

  std::array<std::pair<int, int>, 2> a2;
  a2.at(0) =
      std::make_pair<int, int>(-2, -1);  // only 2 bins left of requested bin
                                         // (not including the requested bin)
  a2.at(1) = std::make_pair<int, int>(
      -1, 2);  // one bin left of requested bin, the requested bin itself and 2
               // bins right of requested bin
  std::set<std::size_t> returnedBins;

  auto returnedBinsVec = g2Cl.neighborHoodIndices({{3, 2}}, a2).collectVector();
  returnedBins.insert(returnedBinsVec.begin(), returnedBinsVec.end());
  std::set<std::size_t> expectedBins{{8, 9, 10, 11, 15, 16, 17, 18}};
  BOOST_CHECK(returnedBins == expectedBins);

  returnedBinsVec = g2Cl.neighborHoodIndices({{1, 5}}, a2).collectVector();
  returnedBins.clear();
  returnedBins.insert(returnedBinsVec.begin(), returnedBinsVec.end());
  expectedBins = {{29, 30, 32, 33, 36, 37, 39, 40}};
  BOOST_CHECK(returnedBins == expectedBins);

  a2.at(0) = {-6, 7};
  a2.at(1) = {0, 0};
  returnedBinsVec = g2Cl.neighborHoodIndices({{1, 5}}, a2).collectVector();
  returnedBins.clear();
  returnedBins.insert(returnedBinsVec.begin(), returnedBinsVec.end());
  expectedBins = {{12, 19, 26, 33, 40}};
  BOOST_CHECK(returnedBins == expectedBins);

  // @TODO 3D test would be nice, but should essentially not be a problem if
  // 2D works.

  // clang-format off
    /*
     *       1   2    3    4    5
     *   |------------------------|
     * 1 |  8 |  9 | 10 | 11 | 12 |
     *   |----|----|----|----|----|
     * 2 | 15 | 16 | 17 | 18 | 19 |
     *   |----|----|----|----|----|
     * 3 | 22 | 23 | 24 | 25 | 26 |
     *   |----|----|----|----|----|
     * 4 | 29 | 30 | 31 | 32 | 33 |
     *   |----|----|----|----|----|
     * 5 | 36 | 37 | 38 | 39 | 40 |
     *   |------------------------|
     */
  // clang-format on
}

BOOST_AUTO_TEST_CASE(closestPoints) {
  using Point = std::array<double, 3>;
  using bins_t = std::vector<std::size_t>;

  Axis a(0.0, 1.0, 10u);
  Axis b(0.0, 1.0, 5u);
  Axis c(0.0, 1.0, 3u);
  Grid g1(Type<double>, Axis{a});
  Grid g2(Type<double>, Axis{a}, Axis{b});
  Grid g3(Type<double>, std::move(a), std::move(b), std::move(c));

  // clang-format off
    // 1D case
    BOOST_CHECK(g1.closestPointsIndices(Point({{0.52}})).collectVector()
                == bins_t({6, 7}));
    BOOST_CHECK(g1.closestPointsIndices(Point({{0.98}})).collectVector()
                == bins_t({10, 11}));

    // 2D case
    BOOST_CHECK(g2.closestPointsIndices(Point({{0.52, 0.08}})).collectVector()
                == bins_t({43, 44, 50, 51}));
    BOOST_CHECK(g2.closestPointsIndices(Point({{0.05, 0.08}})).collectVector()
                == bins_t({8, 9, 15, 16}));

    // 3D case
    BOOST_CHECK(g3.closestPointsIndices(Point({{0.23, 0.13, 0.61}})).collectVector()
                == bins_t({112, 113, 117, 118, 147, 148, 152, 153}));
    BOOST_CHECK(g3.closestPointsIndices(Point({{0.52, 0.35, 0.71}})).collectVector()
                == bins_t({223, 224, 228, 229, 258, 259, 263, 264}));

    using EAxisClosed = Axis<AxisType::Equidistant, AxisBoundaryType::Closed>;
    using Grid1Cl_t = Grid<double, EAxisClosed>;
    using Grid2Cl_t = Grid<double, EAxisClosed, EAxisClosed>;
    //using Grid3Cl_t = Grid<double, EAxisClosed, EAxisClosed, EAxisClosed>;
    EAxisClosed   aCl(0.0, 1.0, 10u);
    EAxisClosed   bCl(0.0, 1.0, 5u);
    // EAxisClosed   cCl(0.0, 1.0, 3u);
    Grid1Cl_t g1Cl(std::make_tuple(aCl));
    Grid2Cl_t g2Cl(std::make_tuple(std::move(aCl), std::move(bCl)));

    // 1D case
    BOOST_CHECK(g1Cl.closestPointsIndices(Point({{0.52}})).collectVector()
                == bins_t({6, 7}));
    BOOST_CHECK(g1Cl.closestPointsIndices(Point({{0.98}})).collectVector()
                == bins_t({10, 1}));

    // 2D case
    BOOST_CHECK(g2Cl.closestPointsIndices(Point({{0.52, 0.08}})).collectVector()
                == bins_t({43, 44, 50, 51}));
    BOOST_CHECK(g2Cl.closestPointsIndices(Point({{0.52, 0.68}})).collectVector()
                == bins_t({46, 47, 53, 54}));
    BOOST_CHECK(g2Cl.closestPointsIndices(Point({{0.52, 0.88}})).collectVector()
                == bins_t({47, 43, 54, 50}));
    BOOST_CHECK(g2Cl.closestPointsIndices(Point({{0.05, 0.08}})).collectVector()
                == bins_t({8, 9, 15, 16}));
    BOOST_CHECK(g2Cl.closestPointsIndices(Point({{0.9, 0.95}})).collectVector()
                == bins_t({75, 71, 12, 8}));

    // @TODO: 3D checks would also be nice

    Axis  aOp(AxisBound,0.0, 1.0, 10u);
    Axis  bOp(AxisBound,0.0, 1.0, 5u);
    Grid g1Op(Type<double>, Axis{aOp});
    Grid g2Op(Type<double>, std::move(aOp), std::move(bOp));

    // 1D case
    BOOST_CHECK(g1Op.closestPointsIndices(Point({{0.52}})).collectVector()
                == bins_t({6, 7}));
    BOOST_CHECK(g1Op.closestPointsIndices(Point({{0.98}})).collectVector()
                == bins_t({10}));
    BOOST_CHECK(g1Op.closestPointsIndices(Point({{0.88}})).collectVector()
                == bins_t({9, 10}));

    // 2D case
    BOOST_CHECK(g2Op.closestPointsIndices(Point({{0.52, 0.08}})).collectVector()
                == bins_t({43, 44, 50, 51}));
    BOOST_CHECK(g2Op.closestPointsIndices(Point({{0.52, 0.68}})).collectVector()
                == bins_t({46, 47, 53, 54}));
    BOOST_CHECK(g2Op.closestPointsIndices(Point({{0.52, 0.88}})).collectVector()
                == bins_t({47, 54}));
    BOOST_CHECK(g2Op.closestPointsIndices(Point({{0.05, 0.1}})).collectVector()
                == bins_t({8, 9, 15, 16}));
    BOOST_CHECK(g2Op.closestPointsIndices(Point({{0.95, 0.95}})).collectVector()
                == bins_t({75}));

    // @TODO: 3D checks would also be nice

    /*
     *       1    2    3    4    5
     *    |------------------------|
     *  1 |  8 |  9 | 10 | 11 | 12 |
     *    |----|----|----|----|----|
     *  2 | 15 | 16 | 17 | 18 | 19 |
     *    |----|----|----|----|----|
     *  3 | 22 | 23 | 24 | 25 | 26 |
     *    |----|----|----|----|----|
     *  4 | 29 | 30 | 31 | 32 | 33 |
     *    |----|----|----|----|----|
     *  5 | 36 | 37 | 38 | 39 | 40 |
     *    |------------------------|
     *  6 | 43 | 44 | 45 | 46 | 47 |
     *    |------------------------|
     *  7 | 50 | 51 | 52 | 53 | 54 |
     *    |------------------------|
     *  8 | 57 | 58 | 59 | 60 | 61 |
     *    |------------------------|
     *  9 | 64 | 65 | 66 | 67 | 68 |
     *    |------------------------|
     * 10 | 71 | 72 | 73 | 74 | 75 |
     *    |------------------------|
     * 77   78   79   80   81   82   83
     */

  // clang-format on
}

BOOST_AUTO_TEST_CASE(grid_type_conversion) {
  // Type conversion test
  using Grid2Int =
      Grid<int, Axis<AxisType::Equidistant>, Axis<AxisType::Variable>>;

  Axis a(0.0, 1.0, 10u);
  Axis b({0., 1.2, 2.3, 3.4, 4.5, 5.6});
  Grid g2(Type<double>, std::move(a), std::move(b));
  decltype(g2) g2Copy(g2.axesTuple());

  static_assert(std::same_as<decltype(g2), decltype(g2Copy)>);

  auto g2ConvertedInt = g2Copy.convertType<int>();
  static_assert(std::same_as<decltype(g2ConvertedInt), Grid2Int>);
}

BOOST_AUTO_TEST_CASE(grid_full_conversion) {
  // The converter class
  struct DoubleToInt {
    // Declare a value tupe
    using value_type = int;
    // the conversion operator
    int operator()(double d) { return static_cast<int>(d); }
  };

  // Grid conversion test
  Axis a(0.0, 1.0, 2u);
  Grid g1(Type<double>, std::move(a));

  using Point = std::array<double, 1>;
  g1.atPosition(Point({{0.3}})) = 1.1;
  g1.atPosition(Point({{0.6}})) = 2.4;

  DoubleToInt d2i;

  auto g1ConvertedInt = g1.convertGrid(d2i);
  BOOST_CHECK_EQUAL(g1ConvertedInt.atPosition(Point({{0.3}})), 1);
  BOOST_CHECK_EQUAL(g1ConvertedInt.atPosition(Point({{0.6}})), 2);
}

BOOST_AUTO_TEST_CASE(Output) {
  Axis a{AxisOpen, 0.0, 1.0, 10u};
  Axis b{AxisBound, {1, 2, 3}};

  Grid g(Type<double>, std::move(a), std::move(b));

  std::stringstream ss;
  ss << g;
  BOOST_CHECK_EQUAL(
      ss.str(),
      "Axis<Equidistant, Open>(0, 1, 10), Axis<Variable, Bound>(1, 2, 3)");

  const IGrid& ig = g;

  ss.str("");

  ss << ig;

  BOOST_CHECK_EQUAL(
      ss.str(),
      "Axis<Equidistant, Open>(0, 1, 10), Axis<Variable, Bound>(1, 2, 3)");
}

BOOST_AUTO_TEST_CASE(Equality) {
  Axis a{AxisOpen, 0.0, 1.0, 10u};
  Axis b{AxisBound, {1, 2, 3}};
  Axis c{AxisClosed, {1, 2, 5}};

  Grid ab{Type<double>, a, b};
  Grid ac{Type<double>, a, c};

  BOOST_CHECK_EQUAL(ab, ab);
  BOOST_CHECK_EQUAL(ac, ac);
  BOOST_CHECK_NE(ab, ac);

  const IGrid& iab = ab;
  const IGrid& iac = ac;

  BOOST_CHECK_EQUAL(iab, iab);
  BOOST_CHECK_EQUAL(iac, iac);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
