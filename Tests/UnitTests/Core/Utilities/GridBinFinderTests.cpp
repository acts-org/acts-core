// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/GridBinFinder.hpp"
#include "Acts/Utilities/GridIterator.hpp"

#include <array>
#include <utility>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(grid_binfinder_boundTypes) {
  const std::size_t nBins = 10ul;
  Acts::GridBinFinder<1ul> binFinder(1);

  // take a look at the boundaries of the axes
  std::array<std::size_t, 1ul> lowerBound({1ul});
  std::array<std::size_t, 1ul> upperBound({10ul});

  // For Closed Boundary: out-of-bounds lookups wrap-around to the other side of
  // the axis.
  Acts::Axis xAxisClosed(AxisClosed, 0, 100, nBins);
  Acts::Grid gridClosed(Type<double>, std::move(xAxisClosed));

  auto lowerClosedNeighbours = binFinder.findBins(lowerBound, gridClosed);
  BOOST_CHECK_EQUAL(lowerClosedNeighbours.size(), 3ul);
  BOOST_CHECK_EQUAL(lowerClosedNeighbours[0ul], 10ul);
  BOOST_CHECK_EQUAL(lowerClosedNeighbours[1ul], 1ul);
  BOOST_CHECK_EQUAL(lowerClosedNeighbours[2ul], 2ul);

  auto upperClosedNeighbours = binFinder.findBins(upperBound, gridClosed);
  BOOST_CHECK_EQUAL(upperClosedNeighbours.size(), 3ul);
  BOOST_CHECK_EQUAL(upperClosedNeighbours[0ul], 9ul);
  BOOST_CHECK_EQUAL(upperClosedNeighbours[1ul], 10ul);
  BOOST_CHECK_EQUAL(upperClosedNeighbours[2ul], 1ul);

  // For Open Boundary [default]: out-of-bounds lookups resolve to dedicated
  // underflow and overflow bins.
  Acts::Axis xAxisOpen(AxisOpen, 0, 100, nBins);
  Acts::Grid gridOpen(Type<double>, std::move(xAxisOpen));

  auto lowerOpenNeighbours = binFinder.findBins(lowerBound, gridOpen);
  BOOST_CHECK_EQUAL(lowerOpenNeighbours.size(), 3ul);
  BOOST_CHECK_EQUAL(lowerOpenNeighbours[0ul], 0ul);
  BOOST_CHECK_EQUAL(lowerOpenNeighbours[1ul], 1ul);
  BOOST_CHECK_EQUAL(lowerOpenNeighbours[2ul], 2ul);

  auto upperOpenNeighbours = binFinder.findBins(upperBound, gridOpen);
  BOOST_CHECK_EQUAL(upperOpenNeighbours.size(), 3ul);
  BOOST_CHECK_EQUAL(upperOpenNeighbours[0ul], 9ul);
  BOOST_CHECK_EQUAL(upperOpenNeighbours[1ul], 10ul);
  BOOST_CHECK_EQUAL(upperOpenNeighbours[2ul], 11ul);

  // For Bound Boundary: out-of-bounds lookups resolve to the closest valid bin.
  Acts::Axis xAxisBound(AxisBound, 0, 100, nBins);
  Acts::Grid gridBound(Type<double>, std::move(xAxisBound));

  auto lowerBoundNeighbours = binFinder.findBins(lowerBound, gridBound);
  BOOST_CHECK_EQUAL(lowerBoundNeighbours.size(), 2ul);
  BOOST_CHECK_EQUAL(lowerBoundNeighbours[0ul], 1ul);
  BOOST_CHECK_EQUAL(lowerBoundNeighbours[1ul], 2ul);

  auto upperBoundNeighbours = binFinder.findBins(upperBound, gridBound);
  BOOST_CHECK_EQUAL(upperBoundNeighbours.size(), 2ul);
  BOOST_CHECK_EQUAL(upperBoundNeighbours[0ul], 9ul);
  BOOST_CHECK_EQUAL(upperBoundNeighbours[1ul], 10ul);
}

BOOST_AUTO_TEST_CASE(grid_binfinder_constructor) {
  using list_t = std::vector<std::pair<int, int>>;
  Acts::GridBinFinder<1ul> binFinder_1d_1(1);
  Acts::GridBinFinder<1ul> binFinder_1d_2(list_t({}));
  Acts::GridBinFinder<1ul> binFinder_1d_3(list_t({{0, 2}, {-1, 1}}));
  Acts::GridBinFinder<1ul> binFinder_1d_4(std::make_pair(1, 1));

  Acts::GridBinFinder<2ul> binFinder_2d_1(1, 5);
  Acts::GridBinFinder<2ul> binFinder_2d_2(list_t({}),
                                          list_t({{0, 2}, {-1, 1}}));
  Acts::GridBinFinder<2ul> binFinder_2d_3(list_t({}), 2);
  Acts::GridBinFinder<2ul> binFinder_2d_4(std::make_pair(1, 2), 2);

  Acts::GridBinFinder<3ul> binFinder_3d_1(1, 1, 5);

  Acts::GridBinFinder<10ul> binFinder_10d_1(1, 1, 5, 0, 4, 2, 3, 1, 1, 9);
}

BOOST_AUTO_TEST_CASE(grid_binfinder_test_1d_ints) {
  const std::size_t nBins = 10ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Grid grid(Type<double>, std::move(xAxis));

  std::array<std::size_t, 1ul> locPosition({3ul});

  Acts::GridBinFinder<1ul> binFinder_1(1);
  auto neighbours_1 = binFinder_1.findBins(locPosition, grid);
  BOOST_CHECK_EQUAL(neighbours_1.size(), 3ul);

  for (const std::size_t neighbour : neighbours_1) {
    std::array<std::size_t, 1ul> neighboutLocPosition =
        grid.localBinsFromGlobalBin(neighbour);
    std::size_t distance = locPosition[0ul] <= neighboutLocPosition[0ul]
                               ? neighboutLocPosition[0ul] - locPosition[0ul]
                               : locPosition[0ul] - neighboutLocPosition[0ul];
    BOOST_CHECK(distance <= 1ul);
  }

  Acts::GridBinFinder<1ul> binFinder_2(2);
  auto neighbours_2 = binFinder_2.findBins(locPosition, grid);
  BOOST_CHECK_EQUAL(neighbours_2.size(), 5ul);

  for (const std::size_t neighbour : neighbours_2) {
    std::array<std::size_t, 1ul> neighboutLocPosition =
        grid.localBinsFromGlobalBin(neighbour);
    std::size_t distance = locPosition[0ul] <= neighboutLocPosition[0ul]
                               ? neighboutLocPosition[0ul] - locPosition[0ul]
                               : locPosition[0ul] - neighboutLocPosition[0ul];
    BOOST_CHECK(distance <= 2ul);
  }
}

BOOST_AUTO_TEST_CASE(grid_binfinder_test_2d_ints) {
  const std::size_t nBinsX = 10ul;
  const std::size_t nBinsY = 10ul;
  Acts::Axis xAxis(0, 100, nBinsX);
  Acts::Axis yAxis(0, 100, nBinsY);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis));

  std::array<std::size_t, 2ul> locPosition({3ul, 6ul});

  Acts::GridBinFinder<2ul> binFinder_1(1, 3);
  std::array<std::size_t, 2ul> dims_1({1, 3});
  auto neighbours_1 = binFinder_1.findBins(locPosition, grid);
  BOOST_CHECK_EQUAL(neighbours_1.size(), 3ul * 7ul);

  for (const std::size_t neighbour : neighbours_1) {
    std::array<std::size_t, 2ul> neighboutLocPosition =
        grid.localBinsFromGlobalBin(neighbour);
    for (std::size_t dim(0ul); dim < 2ul; ++dim) {
      std::size_t distance = locPosition[dim] <= neighboutLocPosition[dim]
                                 ? neighboutLocPosition[dim] - locPosition[dim]
                                 : locPosition[dim] - neighboutLocPosition[dim];
      BOOST_CHECK(distance <= dims_1[dim]);
    }
  }

  Acts::GridBinFinder<2ul> binFinder_2(2, 1);
  std::array<std::size_t, 2ul> dims_2({2, 1});
  auto neighbours_2 = binFinder_2.findBins(locPosition, grid);
  BOOST_CHECK_EQUAL(neighbours_2.size(), 5ul * 3ul);

  for (const std::size_t neighbour : neighbours_2) {
    std::array<std::size_t, 2ul> neighboutLocPosition =
        grid.localBinsFromGlobalBin(neighbour);
    for (std::size_t dim(0ul); dim < 2ul; ++dim) {
      std::size_t distance = locPosition[dim] <= neighboutLocPosition[dim]
                                 ? neighboutLocPosition[dim] - locPosition[dim]
                                 : locPosition[dim] - neighboutLocPosition[dim];
      BOOST_CHECK(distance <= dims_2[dim]);
    }
  }
}

BOOST_AUTO_TEST_CASE(grid_binfinder_test_3d_ints) {
  const std::size_t nBinsX = 10ul;
  const std::size_t nBinsY = 10ul;
  const std::size_t nBinsZ = 3ul;
  Acts::Axis xAxis(0, 100, nBinsX);
  Acts::Axis yAxis(0, 100, nBinsY);
  Acts::Axis zAxis(0, 100, nBinsZ);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis),
                  std::move(zAxis));

  std::array<std::size_t, 3ul> locPosition({3ul, 6ul, 2ul});

  Acts::GridBinFinder<3ul> binFinder(1, 2, 0);
  std::array<std::size_t, 3ul> dims({1, 2, 0});
  auto neighbours = binFinder.findBins(locPosition, grid);
  BOOST_CHECK_EQUAL(neighbours.size(), 3ul * 5ul * 1ul);

  for (const std::size_t neighbour : neighbours) {
    std::array<std::size_t, 3ul> neighboutLocPosition =
        grid.localBinsFromGlobalBin(neighbour);
    for (std::size_t dim(0ul); dim < 3ul; ++dim) {
      std::size_t distance = locPosition[dim] <= neighboutLocPosition[dim]
                                 ? neighboutLocPosition[dim] - locPosition[dim]
                                 : locPosition[dim] - neighboutLocPosition[dim];
      BOOST_CHECK(distance <= dims[dim]);
    }
  }
}

BOOST_AUTO_TEST_CASE(grid_binfinder_test_1d_pair) {
  const std::size_t nBins = 10ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Grid grid(Type<double>, std::move(xAxis));

  std::array<std::size_t, 1ul> locPosition({3ul});

  Acts::GridBinFinder<1ul> binFinder_1(std::make_pair(1, 1));
  auto neighbours_1 = binFinder_1.findBins(locPosition, grid);
  BOOST_CHECK_EQUAL(neighbours_1.size(), 3ul);

  for (const std::size_t neighbour : neighbours_1) {
    std::array<std::size_t, 1ul> neighboutLocPosition =
        grid.localBinsFromGlobalBin(neighbour);
    std::size_t distance = locPosition[0ul] <= neighboutLocPosition[0ul]
                               ? neighboutLocPosition[0ul] - locPosition[0ul]
                               : locPosition[0ul] - neighboutLocPosition[0ul];
    BOOST_CHECK(distance <= 1ul);
  }

  Acts::GridBinFinder<1ul> binFinder_2(std::make_pair(2, 2));
  auto neighbours_2 = binFinder_2.findBins(locPosition, grid);
  BOOST_CHECK_EQUAL(neighbours_2.size(), 5ul);

  for (const std::size_t neighbour : neighbours_2) {
    std::array<std::size_t, 1ul> neighboutLocPosition =
        grid.localBinsFromGlobalBin(neighbour);
    std::size_t distance = locPosition[0ul] <= neighboutLocPosition[0ul]
                               ? neighboutLocPosition[0ul] - locPosition[0ul]
                               : locPosition[0ul] - neighboutLocPosition[0ul];
    BOOST_CHECK(distance <= 2ul);
  }
}

BOOST_AUTO_TEST_CASE(grid_binfinder_test_1d_pair_asymmetric) {
  const std::size_t nBins = 10ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Grid grid(Type<double>, std::move(xAxis));

  std::array<std::size_t, 1ul> locPosition({3ul});

  Acts::GridBinFinder<1ul> binFinder_1(std::make_pair(1, 2));
  auto neighbours_1 = binFinder_1.findBins(locPosition, grid);
  BOOST_CHECK_EQUAL(neighbours_1.size(), 4ul);

  std::array<std::size_t, 4ul> expected({2ul, 3ul, 4ul, 5ul});
  for (std::size_t i(0ul); i < 4ul; ++i) {
    BOOST_CHECK_EQUAL(neighbours_1[i], expected[i]);
  }
}

BOOST_AUTO_TEST_CASE(grid_binfinder_test_2d_pair) {
  const std::size_t nBinsX = 10ul;
  const std::size_t nBinsY = 10ul;
  Acts::Axis xAxis(0, 100, nBinsX);
  Acts::Axis yAxis(0, 100, nBinsY);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis));

  std::array<std::size_t, 2ul> locPosition({3ul, 6ul});

  Acts::GridBinFinder<2ul> binFinder_1(std::make_pair(1, 1),
                                       std::make_pair(3, 3));
  std::array<std::size_t, 2ul> dims_1({1, 3});
  auto neighbours_1 = binFinder_1.findBins(locPosition, grid);
  BOOST_CHECK_EQUAL(neighbours_1.size(), 3ul * 7ul);

  for (const std::size_t neighbour : neighbours_1) {
    std::array<std::size_t, 2ul> neighboutLocPosition =
        grid.localBinsFromGlobalBin(neighbour);
    for (std::size_t dim(0ul); dim < 2ul; ++dim) {
      std::size_t distance = locPosition[dim] <= neighboutLocPosition[dim]
                                 ? neighboutLocPosition[dim] - locPosition[dim]
                                 : locPosition[dim] - neighboutLocPosition[dim];
      BOOST_CHECK(distance <= dims_1[dim]);
    }
  }

  Acts::GridBinFinder<2ul> binFinder_2(std::make_pair(2, 2),
                                       std::make_pair(1, 1));
  std::array<std::size_t, 2ul> dims_2({2, 1});
  auto neighbours_2 = binFinder_2.findBins(locPosition, grid);
  BOOST_CHECK_EQUAL(neighbours_2.size(), 5ul * 3ul);

  for (const std::size_t neighbour : neighbours_2) {
    std::array<std::size_t, 2ul> neighboutLocPosition =
        grid.localBinsFromGlobalBin(neighbour);
    for (std::size_t dim(0ul); dim < 2ul; ++dim) {
      std::size_t distance = locPosition[dim] <= neighboutLocPosition[dim]
                                 ? neighboutLocPosition[dim] - locPosition[dim]
                                 : locPosition[dim] - neighboutLocPosition[dim];
      BOOST_CHECK(distance <= dims_2[dim]);
    }
  }
}

BOOST_AUTO_TEST_CASE(grid_binfinder_test_1d_pattern) {
  const std::size_t nBins = 5ul;
  Acts::Axis xAxis(0, 100, nBins);
  Acts::Grid grid(Type<double>, std::move(xAxis));

  std::array<std::vector<std::size_t>, 1ul> navigation;
  navigation[0ul].resize(nBins);
  std::iota(navigation[0ul].begin(), navigation[0ul].end(), 1ul);

  std::vector<std::pair<int, int>> neighbours;
  neighbours.push_back(std::make_pair(0, 2));
  neighbours.push_back(std::make_pair(-1, 1));
  neighbours.push_back(std::make_pair(-1, 2));
  neighbours.push_back(std::make_pair(-2, 1));
  neighbours.push_back(std::make_pair(-1, 0));

  BOOST_CHECK_EQUAL(neighbours.size(), grid.numLocalBins()[0ul]);

  auto startGrid = grid.begin(navigation);
  auto stopGrid = grid.end(navigation);

  Acts::GridBinFinder<1ul> binFinder(std::move(neighbours));

  std::size_t counter = 0ul;
  std::vector<std::size_t> expectedNeighbours = {3, 3, 4, 4, 2};

  for (; startGrid != stopGrid; startGrid++) {
    std::array<std::size_t, 1ul> locPosition = startGrid.localBinsIndices();
    auto all_neigh = binFinder.findBins(locPosition, grid);
    BOOST_CHECK_EQUAL(all_neigh.size(), expectedNeighbours[counter++]);
  }

  std::vector<std::pair<int, int>> anotherNeighbours;
  anotherNeighbours.push_back(std::make_pair(1, 2));
  anotherNeighbours.push_back(std::make_pair(-1, 1));
  anotherNeighbours.push_back(std::make_pair(-1, 2));
  anotherNeighbours.push_back(std::make_pair(-2, 1));
  anotherNeighbours.push_back(std::make_pair(-1, 0));

  Acts::GridBinFinder<1ul> anotherBinFinder(std::move(anotherNeighbours));
  std::array<std::size_t, 1ul> locPosition = {1ul};

  auto neighs = anotherBinFinder.findBins(locPosition, grid);
  BOOST_CHECK_EQUAL(neighs.size(), 2ul);

  for (const std::size_t neighbour : neighs) {
    std::array<std::size_t, 1ul> neighboutLocPosition =
        grid.localBinsFromGlobalBin(neighbour);
    std::size_t distance = locPosition[0ul] <= neighboutLocPosition[0ul]
                               ? neighboutLocPosition[0ul] - locPosition[0ul]
                               : locPosition[0ul] - neighboutLocPosition[0ul];
    BOOST_CHECK(distance <= 2ul);
    BOOST_CHECK(distance >= 1ul);
  }
}

BOOST_AUTO_TEST_CASE(grid_binfinder_test_2d_pattern) {
  const std::size_t nBinsX = 5ul;
  const std::size_t nBinsY = 3ul;
  Acts::Axis xAxis(0, 100, nBinsX);
  Acts::Axis yAxis(0, 100, nBinsY);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis));

  std::array<std::vector<std::size_t>, 2ul> navigation;
  navigation[0ul].resize(nBinsX);
  navigation[1ul].resize(nBinsY);
  std::iota(navigation[0ul].begin(), navigation[0ul].end(), 1ul);
  std::iota(navigation[1ul].begin(), navigation[1ul].end(), 1ul);

  std::vector<std::pair<int, int>> neighboursX;
  neighboursX.push_back(std::make_pair(0, 2));
  neighboursX.push_back(std::make_pair(-1, 1));
  neighboursX.push_back(std::make_pair(-1, 2));
  neighboursX.push_back(std::make_pair(-2, 1));
  neighboursX.push_back(std::make_pair(-1, 0));

  std::vector<std::pair<int, int>> neighboursY;
  neighboursY.push_back(std::make_pair(0, 1));
  neighboursY.push_back(std::make_pair(-1, 1));
  neighboursY.push_back(std::make_pair(-1, 0));

  BOOST_CHECK_EQUAL(neighboursX.size(), grid.numLocalBins()[0ul]);
  BOOST_CHECK_EQUAL(neighboursY.size(), grid.numLocalBins()[1ul]);

  auto startGrid = grid.begin(navigation);
  auto stopGrid = grid.end(navigation);

  std::size_t counter = 0ul;
  std::vector<std::size_t> expectedNeighbours = {6, 9, 6,  6, 9, 6, 8, 12,
                                                 8, 8, 12, 8, 4, 6, 4};

  BOOST_CHECK_EQUAL(expectedNeighbours.size(),
                    neighboursX.size() * neighboursY.size());

  Acts::GridBinFinder<2ul> binFinder(std::move(neighboursX),
                                     std::move(neighboursY));

  for (; startGrid != stopGrid; startGrid++) {
    std::array<std::size_t, 2ul> locPosition = startGrid.localBinsIndices();
    auto all_neigh = binFinder.findBins(locPosition, grid);
    BOOST_CHECK_EQUAL(all_neigh.size(), expectedNeighbours[counter++]);
  }
}

BOOST_AUTO_TEST_CASE(grid_binfinder_test_2d_empty_pattern) {
  const std::size_t nBinsX = 5ul;
  const std::size_t nBinsY = 3ul;
  Acts::Axis xAxis(0, 100, nBinsX);
  Acts::Axis yAxis(0, 100, nBinsY);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis));

  std::array<std::vector<std::size_t>, 2ul> navigation;
  navigation[0ul].resize(nBinsX);
  navigation[1ul].resize(nBinsY);
  std::iota(navigation[0ul].begin(), navigation[0ul].end(), 1ul);
  std::iota(navigation[1ul].begin(), navigation[1ul].end(), 1ul);

  std::vector<std::pair<int, int>> neighboursX;
  std::vector<std::pair<int, int>> neighboursY;

  auto startGrid = grid.begin(navigation);
  auto stopGrid = grid.end(navigation);

  Acts::GridBinFinder<2ul> binFinder(std::move(neighboursX),
                                     std::move(neighboursY));

  for (; startGrid != stopGrid; startGrid++) {
    std::array<std::size_t, 2ul> locPosition = startGrid.localBinsIndices();
    auto all_neigh = binFinder.findBins(locPosition, grid);
    BOOST_CHECK_EQUAL(all_neigh.size(), 9ul);
  }
}

BOOST_AUTO_TEST_CASE(grid_binfinder_test_2d_mixed) {
  const std::size_t nBinsX = 5ul;
  const std::size_t nBinsY = 3ul;
  Acts::Axis xAxis(0, 100, nBinsX);
  Acts::Axis yAxis(0, 100, nBinsY);
  Acts::Grid grid(Type<double>, std::move(xAxis), std::move(yAxis));

  std::array<std::vector<std::size_t>, 2ul> navigation;
  navigation[0ul].resize(nBinsX);
  navigation[1ul].resize(nBinsY);
  std::iota(navigation[0ul].begin(), navigation[0ul].end(), 1ul);
  std::iota(navigation[1ul].begin(), navigation[1ul].end(), 1ul);

  std::vector<std::pair<int, int>> neighboursX;
  neighboursX.push_back(std::make_pair(0, 2));
  neighboursX.push_back(std::make_pair(-1, 1));
  neighboursX.push_back(std::make_pair(-1, 2));
  neighboursX.push_back(std::make_pair(-2, 1));
  neighboursX.push_back(std::make_pair(-1, 0));

  BOOST_CHECK_EQUAL(neighboursX.size(), grid.numLocalBins()[0ul]);

  auto startGrid = grid.begin(navigation);
  auto stopGrid = grid.end(navigation);

  std::size_t counter = 0ul;
  std::vector<std::size_t> expectedNeighbours = {9,  9,  9,  9,  9, 9, 12, 12,
                                                 12, 12, 12, 12, 6, 6, 6};

  BOOST_CHECK_EQUAL(expectedNeighbours.size(), neighboursX.size() * nBinsY);

  Acts::GridBinFinder<2ul> binFinder(std::move(neighboursX), 1);

  for (; startGrid != stopGrid; startGrid++) {
    std::array<std::size_t, 2ul> locPosition = startGrid.localBinsIndices();
    auto all_neigh = binFinder.findBins(locPosition, grid);
    BOOST_CHECK_EQUAL(all_neigh.size(), expectedNeighbours[counter++]);
  }
}

}  // namespace Acts::Test
