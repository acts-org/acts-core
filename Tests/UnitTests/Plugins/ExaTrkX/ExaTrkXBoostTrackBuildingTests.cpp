// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/ExaTrkX/BoostTrackBuilding.hpp"
#include "Acts/Plugins/ExaTrkX/detail/TensorVectorConversion.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>

BOOST_AUTO_TEST_CASE(test_track_building) {
  // Make some spacepoint IDs
  // The spacepoint ids are [100, 101, 102, ...]
  // They should not be zero based to check if the thing also works if the
  // spacepoint IDs do not match the node IDs used for the edges
  std::vector<int> spacepointIds(16);
  std::iota(spacepointIds.begin(), spacepointIds.end(), 100);

  // Build 4 tracks with 4 hits
  std::vector<std::vector<int>> refTracks;
  for (auto t = 0ul; t < 4; ++t) {
    refTracks.emplace_back(spacepointIds.begin() + 4 * t,
                           spacepointIds.begin() + 4 * (t + 1));
  }

  // Make edges
  std::vector<std::int64_t> edges;
  for (const auto &track : refTracks) {
    for (auto it = track.begin(); it != track.end() - 1; ++it) {
      // edges must be 0 based, so subtract 100 again
      edges.push_back(*it - 100);
      edges.push_back(*std::next(it) - 100);
    }
  }

  auto edgeTensor =
      Acts::detail::vectorToTensor2D(edges, 2).t().contiguous().clone();
  auto dummyWeights = torch::ones(edges.size() / 2, torch::kFloat32);

  // Run Track building
  auto logger = Acts::getDefaultLogger("TestLogger", Acts::Logging::ERROR);
  Acts::BoostTrackBuilding trackBuilder(std::move(logger));

  auto testTracks = trackBuilder({}, edgeTensor, dummyWeights, spacepointIds);

  // Sort tracks, so we can find them
  std::for_each(testTracks.begin(), testTracks.end(),
                [](auto &t) { std::ranges::sort(t); });
  std::for_each(refTracks.begin(), refTracks.end(),
                [](auto &t) { std::ranges::sort(t); });

  // Check what we have here
  for (const auto &refTrack : refTracks) {
    BOOST_CHECK(Acts::rangeContainsValue(testTracks, refTrack));
  }
}
