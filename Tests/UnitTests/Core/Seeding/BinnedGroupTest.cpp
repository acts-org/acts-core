// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Seeding/BinnedGroup.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/GridBinFinder.hpp"
#include "Acts/Utilities/GridIterator.hpp"

#include <array>
#include <memory>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(binned_group_constructor) {
  Acts::Axis xAxis(0, 100, 10);
  Acts::Axis yAxis(0, 100, 10);
  Acts::Axis zAxis(0, 100, 10);

  constexpr auto data_type = Type<std::vector<std::size_t>>;
  Grid grid_1d(data_type, xAxis);
  using grid_1d_t = decltype(grid_1d);
  Grid grid_2d(data_type, xAxis, yAxis);
  using grid_2d_t = decltype(grid_2d);
  Grid grid_3d(data_type, xAxis, yAxis, zAxis);
  using grid_3d_t = decltype(grid_3d);

  Acts::GridBinFinder<1ul> binFinder_1d(1);
  Acts::GridBinFinder<2ul> binFinder_2d(1, 1);
  Acts::GridBinFinder<3ul> binFinder_3d(1, 2, 1);

  std::array<std::vector<std::size_t>, 1ul> navigation_1d;
  navigation_1d[0ul].resize(10);
  std::array<std::vector<std::size_t>, 2ul> navigation_2d;
  navigation_2d[1ul].resize(10);

  // Costructors
  // We provide a proper navigation
  Acts::BinnedGroup<grid_1d_t> group_1d(std::move(grid_1d), binFinder_1d,
                                        binFinder_1d, std::move(navigation_1d));
  // We provide a partial navigation, the constructor will complete it
  Acts::BinnedGroup<grid_2d_t> group_2d(std::move(grid_2d), binFinder_2d,
                                        binFinder_2d, std::move(navigation_2d));
  // We do not provide navigation, the constructor will define it
  Acts::BinnedGroup<grid_3d_t> group_3d(std::move(grid_3d), binFinder_3d,
                                        binFinder_3d);

  // Move Constructor/Assignment
  const Acts::BinnedGroup<grid_1d_t> group_1d_moved(std::move(group_1d));
  const Acts::BinnedGroup<grid_2d_t> group_2d_moved(std::move(group_2d));
  const Acts::BinnedGroup<grid_3d_t> group_3d_moved = std::move(group_3d);

  [[maybe_unused]] const grid_1d_t& retrievedGrid = group_1d.grid();
}

BOOST_AUTO_TEST_CASE(binned_group_iterations_1d_emptyGrid) {
  using binfinder_t = Acts::GridBinFinder<1ul>;

  Acts::Axis xAxis(0, 100, 10);
  Grid grid(Type<std::vector<std::size_t>>, std::move(xAxis));
  using grid_t = decltype(grid);
  binfinder_t binfinder(0);
  Acts::BinnedGroup<grid_t> group(std::move(grid), binfinder, binfinder);

  Acts::BinnedGroupIterator<grid_t> itrStart = group.begin();
  Acts::BinnedGroupIterator<grid_t> itrStop = group.end();

  std::size_t nIterations = 0ul;
  for (; itrStart != itrStop; ++itrStart, ++nIterations) {
    [[maybe_unused]] auto candidates = *itrStart;
  }

  BOOST_CHECK_EQUAL(nIterations, 0ul);
}

BOOST_AUTO_TEST_CASE(binned_group_iterations_2d_emptyGrid) {
  using binfinder_t = Acts::GridBinFinder<2ul>;

  Acts::Axis xAxis(0, 100, 10);
  Acts::Axis yAxis(0, 100, 10);
  Grid grid(Type<std::vector<std::size_t>>, std::move(xAxis), std::move(yAxis));
  using grid_t = decltype(grid);
  binfinder_t binfinder(0, 0);
  Acts::BinnedGroup<grid_t> group(std::move(grid), binfinder, binfinder);

  Acts::BinnedGroupIterator<grid_t> itrStart = group.begin();
  Acts::BinnedGroupIterator<grid_t> itrStop = group.end();

  std::size_t nIterations = 0ul;
  for (; itrStart != itrStop; ++itrStart, ++nIterations) {
    [[maybe_unused]] auto candidates = *itrStart;
  }

  BOOST_CHECK_EQUAL(nIterations, 0ul);
}

BOOST_AUTO_TEST_CASE(binned_group_iterations_1d_perFilledGrid) {
  using binfinder_t = Acts::GridBinFinder<1ul>;

  Acts::Axis xAxis(0, 100, 10);
  Grid grid(Type<std::vector<std::size_t>>, xAxis);
  using grid_t = decltype(grid);
  /// Add some entries to the grid
  grid.at(1ul).push_back(4ul);
  grid.at(1ul).push_back(1ul);
  grid.at(8ul).push_back(7ul);
  grid.at(9ul).push_back(2ul);

  binfinder_t botBinfinder(0);
  binfinder_t topBinfinder(1);
  Acts::BinnedGroup<grid_t> group(std::move(grid), botBinfinder, topBinfinder);

  std::size_t nIterations = 0ul;
  for (const auto [bottom, middle, top] : group) {
    ++nIterations;
    BOOST_CHECK_EQUAL(bottom.size(), 1ul);
    BOOST_CHECK_EQUAL(top.size(), 3ul);
  }

  BOOST_CHECK_EQUAL(nIterations, 3ul);
}

BOOST_AUTO_TEST_CASE(binned_group_iterations_2d_perFilledGrid) {
  using binfinder_t = Acts::GridBinFinder<2ul>;

  Acts::Axis xAxis(0, 100, 10);
  Acts::Axis yAxis(0, 100, 10);
  Grid grid(Type<std::vector<std::size_t>>, xAxis, yAxis);
  using grid_t = decltype(grid);
  /// Add some entries to the grid
  grid.atLocalBins({2ul, 4ul}).push_back(4ul);
  grid.atLocalBins({4ul, 4ul}).push_back(4ul);

  binfinder_t botBinfinder(1, 2);
  binfinder_t topBinfinder(1, 1);
  Acts::BinnedGroup<grid_t> group(std::move(grid), botBinfinder, topBinfinder);

  std::size_t nIterations = 0ul;
  for (const auto [bottom, middle, top] : group) {
    ++nIterations;
    BOOST_CHECK_EQUAL(bottom.size(), 15ul);
    BOOST_CHECK_EQUAL(top.size(), 9ul);
  }

  BOOST_CHECK_EQUAL(nIterations, 2ul);
}

BOOST_AUTO_TEST_CASE(binned_group_fill_2d) {
  using value_t = std::size_t;
  using binfinder_t = Acts::GridBinFinder<2ul>;

  Axis phiAxis(AxisClosed, -M_PI, M_PI, 40);
  Axis zAxis(AxisBound, 0, 100, 10);

  Grid grid(Type<std::vector<value_t>>, std::move(phiAxis), std::move(zAxis));
  using grid_t = decltype(grid);
  binfinder_t binfinder(1, 1);
  Acts::BinnedGroup<grid_t> group(std::move(grid), binfinder, binfinder);

  /// Fill the grid already owned by the group filling only one bin at a
  /// specific local position
  std::array<std::size_t, 2ul> locPosition({4ul, 6ul});
  std::size_t globPos = group.grid().globalBinFromLocalBins(locPosition);

  grid_t& storedGrid = group.grid();
  for (std::size_t i(0ul); i < 30ul; ++i) {
    storedGrid.at(globPos).push_back(1ul);
  }

  std::size_t nIterations = 0ul;
  for (const auto [bottom, middle, top] : group) {
    ++nIterations;
    const auto& coll = group.grid().at(middle);
    BOOST_CHECK_EQUAL(coll.size(), 30ul);
  }

  BOOST_CHECK_EQUAL(nIterations, 1ul);
}

BOOST_AUTO_TEST_CASE(binned_group_fill_3d) {
  using value_t = std::size_t;
  using phiAxis_t = Acts::Axis<AxisType::Equidistant, AxisBoundaryType::Closed>;
  using zAxis_t = Axis<AxisType::Equidistant, AxisBoundaryType::Bound>;
  using rAxis_t = Axis<AxisType::Equidistant, AxisBoundaryType::Bound>;
  using grid_t = Acts::Grid<std::vector<value_t>, phiAxis_t, zAxis_t, rAxis_t>;
  using binfinder_t = Acts::GridBinFinder<3ul>;

  phiAxis_t phiAxis(-M_PI, M_PI, 40);
  zAxis_t zAxis(0, 100, 10);
  rAxis_t rAxis(0, 11000, 1);

  grid_t grid(
      std::make_tuple(std::move(phiAxis), std::move(zAxis), std::move(rAxis)));
  binfinder_t binfinder(1, 1, 0);
  Acts::BinnedGroup<grid_t> group(std::move(grid), binfinder, binfinder);

  /// Fill the grid already owned by the group filling only one bin at a
  /// specific local position
  std::array<std::size_t, grid_t::DIM> locPosition({4ul, 6ul, 1ul});
  std::size_t globPos = group.grid().globalBinFromLocalBins(locPosition);

  grid_t& storedGrid = group.grid();
  for (std::size_t i(0ul); i < 30ul; ++i) {
    storedGrid.at(globPos).push_back(1ul);
  }

  std::size_t nIterations = 0ul;
  for (const auto [bottom, middle, top] : group) {
    ++nIterations;
    const auto& coll = group.grid().at(middle);
    BOOST_CHECK_EQUAL(coll.size(), 30ul);
  }

  BOOST_CHECK_EQUAL(nIterations, 1ul);
}

}  // namespace Acts::Test
