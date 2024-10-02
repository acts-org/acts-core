// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/GridIterator.hpp"

#include <array>
#include <unordered_set>

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(grid_iteration_test_1d_global_operators) {
  const std::size_t nBins = 10ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Grid grid(Type<double>, std::move(xAxis));

  BOOST_CHECK_EQUAL(grid.size(true), nBins + 2ul);

  Acts::GridGlobalIterator gridStart = grid.begin();
  Acts::GridGlobalIterator gridStop = grid.end();

  BOOST_CHECK_EQUAL(gridStart == gridStop, false);
  BOOST_CHECK_EQUAL(gridStart != gridStop, true);
  BOOST_CHECK_EQUAL(gridStart < gridStop, true);
  BOOST_CHECK_EQUAL(gridStart <= gridStop, true);
  BOOST_CHECK_EQUAL(gridStart > gridStop, false);
  BOOST_CHECK_EQUAL(gridStart >= gridStop, false);

  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), nBins + 2ul);
  auto itr = gridStart++;
  BOOST_CHECK_EQUAL(std::distance(itr, gridStart), 1ul);
  BOOST_CHECK_EQUAL(std::distance(itr, gridStop), nBins + 2ul);
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), nBins + 1ul);

  itr = ++gridStart;
  BOOST_CHECK_EQUAL(std::distance(itr, gridStart), 0ul);
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), nBins);

  itr = gridStart + std::distance(gridStart, gridStop);
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), nBins);
  BOOST_CHECK_EQUAL(std::distance(itr, gridStop), 0ul);
  BOOST_CHECK_EQUAL(itr == gridStop, true);

  itr = gridStart - 1ul;
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), nBins);
  BOOST_CHECK_EQUAL(std::distance(itr, gridStop), nBins + 1ul);

  gridStart += std::distance(gridStart, gridStop);
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), 0ul);
  BOOST_CHECK_EQUAL(gridStart == gridStop, true);

  gridStart -= 3ul;
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), 3ul);
  BOOST_CHECK_EQUAL(gridStart != gridStop, true);

  [[maybe_unused]] double value = *gridStart;

  Acts::GridGlobalIterator<double, Acts::Axis<AxisType::Equidistant>>
      gridDefault;
  Acts::GridGlobalIterator gridDummy(grid, 0ul);

  BOOST_CHECK_EQUAL(gridDefault == gridDummy, false);
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_2d_global_operators) {
  const std::size_t nBinsX = 10ul;
  const std::size_t nBinsY = 5ul;
  Acts::Axis xAxis(0, 100, nBinsX);
  Acts::Axis yAxis(0, 100, nBinsY);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis));

  BOOST_CHECK_EQUAL(grid.size(true), (nBinsX + 2ul) * (nBinsY + 2ul));

  Acts::GridGlobalIterator gridStart = grid.begin();
  Acts::GridGlobalIterator gridStop = grid.end();

  BOOST_CHECK_EQUAL(gridStart == gridStop, false);
  BOOST_CHECK_EQUAL(gridStart != gridStop, true);
  BOOST_CHECK_EQUAL(gridStart < gridStop, true);
  BOOST_CHECK_EQUAL(gridStart <= gridStop, true);
  BOOST_CHECK_EQUAL(gridStart > gridStop, false);
  BOOST_CHECK_EQUAL(gridStart >= gridStop, false);

  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), grid.size(true));
  auto itr = gridStart++;
  BOOST_CHECK_EQUAL(std::distance(itr, gridStart), 1ul);
  BOOST_CHECK_EQUAL(std::distance(itr, gridStop), grid.size(true));
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), grid.size(true) - 1ul);

  itr = ++gridStart;
  BOOST_CHECK_EQUAL(std::distance(itr, gridStart), 0ul);
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), grid.size(true) - 2ul);

  itr = gridStart + std::distance(gridStart, gridStop);
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), grid.size(true) - 2ul);
  BOOST_CHECK_EQUAL(std::distance(itr, gridStop), 0ul);
  BOOST_CHECK_EQUAL(itr == gridStop, true);

  itr = gridStart - 1ul;
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), grid.size(true) - 2ul);
  BOOST_CHECK_EQUAL(std::distance(itr, gridStop), grid.size(true) - 1ul);

  gridStart += std::distance(gridStart, gridStop);
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), 0ul);
  BOOST_CHECK_EQUAL(gridStart == gridStop, true);

  gridStart -= 3ul;
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), 3ul);
  BOOST_CHECK_EQUAL(gridStart != gridStop, true);

  [[maybe_unused]] double value = *gridStart;

  Acts::GridGlobalIterator<double, Acts::Axis<AxisType::Equidistant>,
                           Acts::Axis<AxisType::Equidistant>>
      gridDefault;
  Acts::GridGlobalIterator gridDummy(grid, 0ul);

  BOOST_CHECK_EQUAL(gridDefault == gridDummy, false);
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_1d_global) {
  const std::size_t nBins = 10ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Grid grid(Type<double>, std::move(xAxis));

  // test general properties
  BOOST_CHECK_EQUAL(grid.size(false), nBins);
  BOOST_CHECK_EQUAL(grid.size(true), nBins + 2ul);

  const std::array<std::size_t, 1ul> numLocalBins = grid.numLocalBins();
  BOOST_CHECK_EQUAL(numLocalBins[0ul], nBins);

  Acts::GridGlobalIterator gridStart = grid.begin();
  Acts::GridGlobalIterator gridStop = grid.end();
  std::size_t numIterations = 0ul;
  for (; gridStart != gridStop; gridStart++) {
    BOOST_CHECK_EQUAL(gridStart.globalBinIndex(), numIterations);
    const std::array<std::size_t, 1ul> locPosition =
        gridStart.localBinsIndices();
    BOOST_CHECK_EQUAL(numIterations, locPosition[0ul]);
    ++numIterations;
  }
  BOOST_CHECK_EQUAL(numIterations, grid.size(true));
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_2d_global) {
  const std::size_t nBins = 10ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Axis yAxis(0, 100, nBins);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis));

  // test general properties
  BOOST_CHECK_EQUAL(grid.size(false), nBins * nBins);
  BOOST_CHECK_EQUAL(grid.size(true), (nBins + 2ul) * (nBins + 2ul));

  const std::array<std::size_t, 2ul> numLocalBins = grid.numLocalBins();
  BOOST_CHECK_EQUAL(numLocalBins[0ul], nBins);
  BOOST_CHECK_EQUAL(numLocalBins[1ul], nBins);

  Acts::GridGlobalIterator gridStart = grid.begin();
  Acts::GridGlobalIterator gridStop = grid.end();
  std::size_t numIterations = 0ul;
  for (; gridStart != gridStop; ++gridStart) {
    BOOST_CHECK_EQUAL(gridStart.globalBinIndex(), numIterations);
    ++numIterations;
  }
  BOOST_CHECK_EQUAL(numIterations, grid.size(true));
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_3d_global) {
  const std::size_t nBins = 10ul;
  const std::size_t nBinsZ = 20ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Axis yAxis(0, 100, nBins);
  Acts::Axis zAxis(0, 100, nBinsZ);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis),
                  std::move(zAxis));

  // test general properties
  BOOST_CHECK_EQUAL(grid.size(false), nBins * nBins * nBinsZ);
  BOOST_CHECK_EQUAL(grid.size(true),
                    (nBins + 2ul) * (nBins + 2ul) * (nBinsZ + 2ul));

  const std::array<std::size_t, 3ul> numLocalBins = grid.numLocalBins();
  BOOST_CHECK_EQUAL(numLocalBins[0ul], nBins);
  BOOST_CHECK_EQUAL(numLocalBins[1ul], nBins);
  BOOST_CHECK_EQUAL(numLocalBins[2ul], nBinsZ);

  Acts::GridGlobalIterator gridStart = grid.begin();
  Acts::GridGlobalIterator gridStop = grid.end();
  std::size_t numIterations = 0ul;
  for (; gridStart != gridStop; ++gridStart) {
    BOOST_CHECK_EQUAL(gridStart.globalBinIndex(), numIterations);
    ++numIterations;
  }
  BOOST_CHECK_EQUAL(numIterations, grid.size(true));
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_1d_local_operators) {
  const std::size_t nBins = 10ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Grid grid(Type<double>, std::move(xAxis));

  std::array<std::vector<std::size_t>, 1ul> navigation;
  navigation[0ul].resize(nBins);
  std::iota(navigation[0ul].begin(), navigation[0ul].end(), 1ul);

  // Constructor without navigation
  Acts::GridLocalIterator gridIterNoNav(grid, {0ul});
  // Constructor(s) with navigation
  Acts::GridLocalIterator gridStart(grid, {0ul}, navigation);

  BOOST_CHECK_EQUAL(std::distance(gridIterNoNav, gridStart), 0ul);
  BOOST_CHECK_EQUAL(gridIterNoNav == gridStart, true);

  Acts::GridLocalIterator gridStop(grid, {nBins}, std::move(navigation));

  BOOST_CHECK_EQUAL(gridStart == gridStop, false);
  BOOST_CHECK_EQUAL(gridStart != gridStop, true);

  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), grid.size(false));
  BOOST_CHECK_EQUAL(gridStart != gridStop, true);

  auto itr = gridStart++;
  BOOST_CHECK_EQUAL(std::distance(itr, gridStart), 1ul);
  BOOST_CHECK_EQUAL(std::distance(itr, gridStop), nBins);
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), nBins - 1ul);

  itr = ++gridStart;
  BOOST_CHECK_EQUAL(std::distance(itr, gridStart), 0ul);
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), nBins - 2ul);

  [[maybe_unused]] double value = *gridStart;
  std::array<std::size_t, 1ul> locPos = gridStart.localBinsIndices();
  BOOST_CHECK_EQUAL(locPos[0ul], 3ul);

  std::size_t globPos = gridStart.globalBinIndex();
  BOOST_CHECK_EQUAL(globPos, 3ul);

  Acts::GridLocalIterator<double, Acts::Axis<AxisType::Equidistant>>
      gridDefault;
  Acts::GridLocalIterator gridDummy(grid, {0ul});

  BOOST_CHECK_EQUAL(gridDefault == gridDummy, false);

  // move operation will invalidate gridStart since the grid gets moved and
  // replaced with a nullptr
  itr = std::move(gridStart);
  BOOST_CHECK_EQUAL(itr == gridStart, false);
  BOOST_CHECK_EQUAL(itr != gridStart, true);
  BOOST_CHECK_EQUAL(std::distance(itr, gridStop), nBins - 2ul);
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_2d_local_operators) {
  const std::size_t nBinsX = 10ul;
  const std::size_t nBinsY = 5ul;
  Acts::Axis xAxis(0, 100, nBinsX);
  Acts::Axis yAxis(0, 100, nBinsY);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis));

  std::array<std::vector<std::size_t>, 2ul> navigation;
  navigation[0ul].resize(nBinsX);
  navigation[1ul].resize(nBinsY);
  std::iota(navigation[0ul].begin(), navigation[0ul].end(), 1ul);
  std::iota(navigation[1ul].begin(), navigation[1ul].end(), 1ul);

  // Constructor without navigation
  Acts::GridLocalIterator gridIterNoNav(grid, {0ul, 0ul});
  // Constructor(s) with navigation
  Acts::GridLocalIterator gridStart(grid, {0ul, 0ul}, navigation);

  BOOST_CHECK_EQUAL(std::distance(gridIterNoNav, gridStart), 0ul);
  BOOST_CHECK_EQUAL(gridIterNoNav == gridStart, true);

  Acts::GridLocalIterator gridStop(grid, {nBinsX, nBinsY},
                                   std::move(navigation));

  BOOST_CHECK_EQUAL(gridStart == gridStop, false);
  BOOST_CHECK_EQUAL(gridStart != gridStop, true);

  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), grid.size(false));
  BOOST_CHECK_EQUAL(gridStart != gridStop, true);

  auto itr = gridStart++;
  BOOST_CHECK_EQUAL(std::distance(itr, gridStart), 1ul);
  BOOST_CHECK_EQUAL(std::distance(itr, gridStop), nBinsX * nBinsY);
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), nBinsX * nBinsY - 1ul);

  itr = ++gridStart;
  BOOST_CHECK_EQUAL(std::distance(itr, gridStart), 0ul);
  BOOST_CHECK_EQUAL(std::distance(gridStart, gridStop), nBinsX * nBinsY - 2ul);

  [[maybe_unused]] double value = *gridStart;
  std::array<std::size_t, 2ul> locPos = gridStart.localBinsIndices();
  BOOST_CHECK_EQUAL(locPos[0ul], 1ul);
  BOOST_CHECK_EQUAL(locPos[1ul], 3ul);

  Acts::GridLocalIterator<double, Acts::Axis<AxisType::Equidistant>,
                          Acts::Axis<AxisType::Equidistant>>
      gridDefault;
  Acts::GridLocalIterator gridDummy(grid, {0ul, 0ul});

  BOOST_CHECK_EQUAL(gridDefault == gridDummy, false);

  // move operation will invalidate gridStart since the grid gets moved and
  // replaced with a nullptr
  itr = std::move(gridStart);
  BOOST_CHECK_EQUAL(itr == gridStart, false);
  BOOST_CHECK_EQUAL(itr != gridStart, true);
  BOOST_CHECK_EQUAL(std::distance(itr, gridStop), nBinsX * nBinsY - 2ul);
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_1d_local_notvalid) {
  const std::size_t nBins = 10ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Grid grid(Type<double>, std::move(xAxis));

  // no navigation bins
  std::array<std::vector<std::size_t>, 1ul> noNavigation;
  BOOST_CHECK_THROW(
      (Acts::GridLocalIterator(grid, {0ul}, std::move(noNavigation))),
      std::invalid_argument);

  // too many steps in the navigation, there are not enough bins in the axis
  std::array<std::vector<std::size_t>, 1ul> tooMuchNavigation;
  tooMuchNavigation[0ul].resize(2 * nBins);
  std::iota(tooMuchNavigation[0ul].begin(), tooMuchNavigation[0ul].end(), 1ul);
  BOOST_CHECK_THROW(
      (Acts::GridLocalIterator(grid, {0ul}, std::move(tooMuchNavigation))),
      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_1d_local) {
  const std::size_t nBins = 10ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Grid grid(Type<double>, std::move(xAxis));

  // test general properties
  BOOST_CHECK_EQUAL(grid.size(false), nBins);
  BOOST_CHECK_EQUAL(grid.size(true), nBins + 2ul);

  const std::array<std::size_t, 1ul> numLocalBins = grid.numLocalBins();
  BOOST_CHECK_EQUAL(numLocalBins[0ul], nBins);

  std::array<std::vector<std::size_t>, 1ul> navigation;
  navigation[0ul].resize(nBins);
  std::iota(navigation[0ul].begin(), navigation[0ul].end(), 1ul);

  Acts::GridLocalIterator gridStart = grid.begin(navigation);
  Acts::GridLocalIterator gridStop = grid.end(navigation);
  std::size_t numIterations = 0ul;
  for (; gridStart != gridStop; ++gridStart) {
    ++numIterations;
  }
  BOOST_CHECK_EQUAL(numIterations, grid.size(false));
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_2d_local) {
  const std::size_t nBins = 10ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Axis yAxis(0, 100, nBins);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis));

  // test general properties
  BOOST_CHECK_EQUAL(grid.size(false), nBins * nBins);
  BOOST_CHECK_EQUAL(grid.size(true), (nBins + 2ul) * (nBins + 2ul));

  const std::array<std::size_t, 2ul> numLocalBins = grid.numLocalBins();
  BOOST_CHECK_EQUAL(numLocalBins[0ul], nBins);
  BOOST_CHECK_EQUAL(numLocalBins[1ul], nBins);

  std::array<std::vector<std::size_t>, 2ul> navigation;
  navigation[0ul].resize(nBins);
  navigation[1ul].resize(nBins);
  for (std::size_t i(0ul); i < 2ul; ++i) {
    std::iota(navigation[i].begin(), navigation[i].end(), 1ul);
  }

  Acts::GridLocalIterator gridStart = grid.begin(navigation);
  Acts::GridLocalIterator gridStop = grid.end(navigation);
  std::size_t numIterations = 0ul;
  for (; gridStart != gridStop; gridStart++) {
    ++numIterations;
  }
  BOOST_CHECK_EQUAL(numIterations, grid.size(false));
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_3d_local) {
  const std::size_t nBins = 10ul;
  const std::size_t nBinsZ = 20ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Axis yAxis(0, 100, nBins);
  Acts::Axis zAxis(0, 100, nBinsZ);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis),
                  std::move(zAxis));

  // test general properties
  BOOST_CHECK_EQUAL(grid.size(false), nBins * nBins * nBinsZ);
  BOOST_CHECK_EQUAL(grid.size(true),
                    (nBins + 2ul) * (nBins + 2ul) * (nBinsZ + 2ul));

  const std::array<std::size_t, 3ul> numLocalBins = grid.numLocalBins();
  BOOST_CHECK_EQUAL(numLocalBins[0ul], nBins);
  BOOST_CHECK_EQUAL(numLocalBins[1ul], nBins);
  BOOST_CHECK_EQUAL(numLocalBins[2ul], nBinsZ);

  std::array<std::vector<std::size_t>, 3ul> navigation;
  navigation[0ul].resize(nBins);
  navigation[1ul].resize(nBins);
  navigation[2ul].resize(nBinsZ);
  for (std::size_t i(0ul); i < 3ul; ++i) {
    std::iota(navigation[i].begin(), navigation[i].end(), 1ul);
  }

  Acts::GridLocalIterator gridStart = grid.begin(navigation);
  Acts::GridLocalIterator gridStop = grid.end(navigation);
  std::size_t numIterations = 0ul;
  for (; gridStart != gridStop; ++gridStart) {
    ++numIterations;
  }
  BOOST_CHECK_EQUAL(numIterations, grid.size(false));
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_3d_local_custom_navigation) {
  const std::size_t nBins = 10ul;
  const std::size_t nBinsZ = 20ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Axis yAxis(0, 100, nBins);
  Acts::Axis zAxis(0, 100, nBinsZ);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis),
                  std::move(zAxis));

  // test general properties
  BOOST_CHECK_EQUAL(grid.size(false), nBins * nBins * nBinsZ);
  BOOST_CHECK_EQUAL(grid.size(true),
                    (nBins + 2ul) * (nBins + 2ul) * (nBinsZ + 2ul));

  const std::array<std::size_t, 3ul> numLocalBins = grid.numLocalBins();
  BOOST_CHECK_EQUAL(numLocalBins[0ul], nBins);
  BOOST_CHECK_EQUAL(numLocalBins[1ul], nBins);
  BOOST_CHECK_EQUAL(numLocalBins[2ul], nBinsZ);

  std::array<std::vector<std::size_t>, 3ul> navigation;
  navigation[0ul] = {1ul, 5ul, 3ul, 2ul, 9ul, 10ul, 4ul, 6ul, 8ul, 7ul};
  navigation[1ul] = {6ul, 8ul, 7ul, 1ul, 5ul, 3ul, 2ul, 9ul, 10ul, 4ul};
  navigation[2ul] = {1ul,  5ul,  3ul,  2ul,  9ul,  10ul, 4ul,
                     6ul,  8ul,  7ul,  11ul, 15ul, 13ul, 12ul,
                     19ul, 20ul, 14ul, 16ul, 18ul, 17ul};

  Acts::GridLocalIterator gridStart = grid.begin(navigation);
  Acts::GridLocalIterator gridStop = grid.end(navigation);
  std::size_t numIterations = 0ul;
  for (; gridStart != gridStop; ++gridStart) {
    ++numIterations;
  }
  BOOST_CHECK_EQUAL(numIterations, grid.size(false));
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_5d_local_custom_subnavigation) {
  const std::size_t nBins = 10ul;
  const std::size_t nBinsZ = 20ul;
  const std::size_t nBinsJK = 5ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Axis yAxis(0, 100, nBins);
  Acts::Axis zAxis(0, 100, nBinsZ);
  Acts::Axis jAxis(0, 100, nBinsJK);
  Acts::Axis kAxis(0, 100, nBinsJK);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis),
                  std::move(zAxis), std::move(jAxis), std::move(kAxis));

  // test general properties
  BOOST_CHECK_EQUAL(grid.size(false),
                    nBins * nBins * nBinsZ * nBinsJK * nBinsJK);
  BOOST_CHECK_EQUAL(grid.size(true), (nBins + 2ul) * (nBins + 2ul) *
                                         (nBinsZ + 2ul) * (nBinsJK + 2ul) *
                                         (nBinsJK + 2ul));

  const std::array<std::size_t, 5ul> numLocalBins = grid.numLocalBins();
  BOOST_CHECK_EQUAL(numLocalBins[0ul], nBins);
  BOOST_CHECK_EQUAL(numLocalBins[1ul], nBins);
  BOOST_CHECK_EQUAL(numLocalBins[2ul], nBinsZ);
  BOOST_CHECK_EQUAL(numLocalBins[3ul], nBinsJK);
  BOOST_CHECK_EQUAL(numLocalBins[4ul], nBinsJK);

  // Iterate only on a few bins
  std::array<std::vector<std::size_t>, 5ul> navigation;
  navigation[0ul] = {1ul, 5ul, 3ul, 2ul, 9ul, 10ul, 4ul, 6ul, 8ul, 7ul};
  navigation[1ul] = {6ul, 8ul, 7ul, 1ul};
  navigation[2ul] = {1ul, 5ul};
  navigation[3ul] = {5ul, 3ul, 2ul};
  navigation[4ul] = {2ul};

  Acts::GridLocalIterator gridStart = grid.begin(navigation);
  Acts::GridLocalIterator gridStop = grid.end(navigation);
  std::size_t numIterations = 0ul;
  for (; gridStart != gridStop; ++gridStart) {
    ++numIterations;
  }

  std::size_t expectedIterations = 1ul;
  for (std::size_t i(0ul); i < 5ul; ++i) {
    expectedIterations *= navigation[i].size();
  }

  BOOST_CHECK_EQUAL(numIterations, expectedIterations);
}

BOOST_AUTO_TEST_CASE(grid_iteration_test_3d_local_norepetitions) {
  const std::size_t nBinsX = 5ul;
  const std::size_t nBinsY = 5ul;
  const std::size_t nBinsZ = 2ul;
  Acts::Axis xAxis(0, 100, nBinsX);
  Acts::Axis yAxis(0, 100, nBinsY);
  Acts::Axis zAxis(0, 100, nBinsZ);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis),
                  std::move(zAxis));

  std::array<std::vector<std::size_t>, 3ul> navigation;
  navigation[0ul] = {1ul, 5ul, 3ul, 2ul, 4ul};
  navigation[1ul] = {4ul, 2ul, 3ul, 5ul, 1ul};
  navigation[2ul] = {2ul, 1ul};

  std::size_t expectedIterations =
      navigation[0ul].size() * navigation[1ul].size() * navigation[2ul].size();

  // Set the allowed values
  std::unordered_set<std::size_t> allowed_global_bins;
  for (std::size_t x : navigation[0ul]) {
    for (std::size_t y : navigation[1ul]) {
      for (std::size_t z : navigation[2ul]) {
        std::array<std::size_t, 3ul> locPos({x, y, z});
        std::size_t globPos = grid.globalBinFromLocalBins(locPos);
        BOOST_CHECK(!allowed_global_bins.contains(globPos));
        allowed_global_bins.insert(globPos);
      }
    }
  }

  BOOST_CHECK_EQUAL(expectedIterations, allowed_global_bins.size());

  Acts::GridLocalIterator gridStart = grid.begin(navigation);
  Acts::GridLocalIterator gridStop = grid.end(navigation);

  // Prepare visited values
  std::unordered_set<std::size_t> visited_global_bins;

  std::size_t numIterations = 0ul;
  for (; gridStart != gridStop; ++gridStart) {
    ++numIterations;
    std::array<std::size_t, 3ul> locPos = gridStart.localBinsIndices();
    std::size_t globPos = grid.globalBinFromLocalBins(locPos);
    BOOST_CHECK(!visited_global_bins.contains(globPos));
    BOOST_CHECK(allowed_global_bins.contains(globPos));
    visited_global_bins.insert(globPos);
  }

  BOOST_CHECK_EQUAL(expectedIterations, numIterations);
  BOOST_CHECK_EQUAL(visited_global_bins.size(), allowed_global_bins.size());
}

}  // namespace Acts::Test
