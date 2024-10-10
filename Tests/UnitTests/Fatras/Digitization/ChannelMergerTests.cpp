// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "ActsFatras/Digitization/ChannelMerger.hpp"
#include "ActsFatras/Digitization/DigitizationData.hpp"

#include <array>
#include <map>
#include <unordered_set>
#include <utility>
#include <vector>

BOOST_AUTO_TEST_SUITE(Digitization)

BOOST_AUTO_TEST_CASE(ChannelMerger1D) {
  // Some cells
  ActsFatras::Cell cell0(5, 10.5);
  ActsFatras::Cell cell1(6, 11.5);
  ActsFatras::Cell cell2(7, 12.5);

  using Channel1D = ActsFatras::Channel<Acts::ActsScalar, 1>;

  // Digital clustering test
  std::vector<Channel1D> channels = {{{cell0}, 1., {5}},
                                     {{cell1}, 1., {5}},
                                     {{cell2}, 1., {5}},
                                     {{cell0}, 0.5, {6}}};

  BOOST_CHECK_EQUAL(channels.size(), 4u);
  auto mergedChannels = ActsFatras::mergeChannels(channels);
  BOOST_CHECK_EQUAL(mergedChannels.size(), 3u);

  std::unordered_set<unsigned int> mergedLinks = {5, 6};
  // Find the merged one and check the merged value
  for (const auto& ch : mergedChannels) {
    if (ch.cellId[0].first == 5) {
      // Check if the channel value is merged
      CHECK_CLOSE_ABS(ch.value, 1.5, Acts::s_epsilon);
      // check if the channel links are merged
      BOOST_CHECK_EQUAL(ch.links.size(), 2u);
      BOOST_CHECK(ch.links == mergedLinks);
    }
  }
}

BOOST_AUTO_TEST_CASE(ChannelMerger2D) {
  // Some cells in 0 direction
  ActsFatras::Cell cell00(5, 10.5);
  ActsFatras::Cell cell01(6, 11.5);
  ActsFatras::Cell cell02(7, 12.5);

  // Some cells in 1 direction
  ActsFatras::Cell cell10(10, 0.5);
  ActsFatras::Cell cell11(10, 0.5);
  ActsFatras::Cell cell12(11, 1.5);

  using Channel2D = ActsFatras::Channel<Acts::ActsScalar, 2>;

  std::vector<Channel2D> channels = {{{cell00, cell10}, 1., {5}},
                                     {{cell01, cell11}, 1., {5}},
                                     {{cell02, cell12}, 1., {5}},
                                     {{cell01, cell11}, 0.5, {6}}};

  BOOST_CHECK_EQUAL(channels.size(), 4u);
  auto mergedChannels = ActsFatras::mergeChannels(channels);
  BOOST_CHECK_EQUAL(mergedChannels.size(), 3u);

  std::unordered_set<unsigned int> mergedLinks = {5, 6};
  // Find the merged one and check the merged value
  for (const auto& ch : mergedChannels) {
    if (ch.cellId[0].first == 6) {
      // Check if the channel value is merged
      CHECK_CLOSE_ABS(ch.value, 1.5, Acts::s_epsilon);
      // check if the channel links are merged
      BOOST_CHECK_EQUAL(ch.links.size(), 2u);
      BOOST_CHECK(ch.links == mergedLinks);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
