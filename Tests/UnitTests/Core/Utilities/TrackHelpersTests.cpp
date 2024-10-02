// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackStateType.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/Utilities/TrackHelpers.hpp"

#include <memory>
#include <span>

namespace Acts::Test {

namespace {

template <typename TrackContainer, typename FlagsPerState>
auto createTestTrack(TrackContainer& tc, const FlagsPerState& flagsPerState) {
  auto t = tc.makeTrack();

  for (const auto& flags : flagsPerState) {
    auto ts = t.appendTrackState();
    for (auto f : flags) {
      ts.typeFlags().set(f);
    }
  }

  return t;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(Utilities)

BOOST_AUTO_TEST_CASE(CalculateQuantities) {
  TrackContainer tc{VectorTrackContainer{}, VectorMultiTrajectory{}};
  auto t = createTestTrack(tc, std::vector<std::vector<TrackStateFlag>>{
                                   {MeasurementFlag},
                                   {OutlierFlag},
                                   {MeasurementFlag, SharedHitFlag},
                                   {HoleFlag},
                                   {OutlierFlag},
                                   {HoleFlag},
                                   {MeasurementFlag, SharedHitFlag},
                                   {OutlierFlag},
                               });

  calculateTrackQuantities(t);

  BOOST_CHECK_EQUAL(t.nHoles(), 2);
  BOOST_CHECK_EQUAL(t.nMeasurements(), 3);
  BOOST_CHECK_EQUAL(t.nOutliers(), 3);
  BOOST_CHECK_EQUAL(t.nSharedHits(), 2);
}

BOOST_AUTO_TEST_CASE(TrimTrack) {
  TrackContainer tc{VectorTrackContainer{}, VectorMultiTrajectory{}};
  auto t = createTestTrack(tc, std::vector<std::vector<TrackStateFlag>>{
                                   {HoleFlag},
                                   {MeasurementFlag},
                                   {OutlierFlag},
                                   {MeasurementFlag, SharedHitFlag},
                                   {HoleFlag},
                                   {OutlierFlag},
                                   {HoleFlag},
                                   {MeasurementFlag},
                                   {OutlierFlag},
                               });

  calculateTrackQuantities(t);

  BOOST_CHECK_EQUAL(t.nHoles(), 3);
  BOOST_CHECK_EQUAL(t.nMeasurements(), 3);
  BOOST_CHECK_EQUAL(t.nOutliers(), 3);
  BOOST_CHECK_EQUAL(t.nSharedHits(), 1);

  trimTrackFront(t, true, true, true);
  calculateTrackQuantities(t);

  BOOST_CHECK_EQUAL(t.nHoles(), 2);
  BOOST_CHECK_EQUAL(t.nMeasurements(), 3);
  BOOST_CHECK_EQUAL(t.nOutliers(), 3);
  BOOST_CHECK_EQUAL(t.nSharedHits(), 1);

  trimTrackBack(t, true, true, true);
  calculateTrackQuantities(t);

  BOOST_CHECK_EQUAL(t.nHoles(), 2);
  BOOST_CHECK_EQUAL(t.nMeasurements(), 3);
  BOOST_CHECK_EQUAL(t.nOutliers(), 2);
  BOOST_CHECK_EQUAL(t.nSharedHits(), 1);

  trimTrack(t, true, true, true);
  calculateTrackQuantities(t);

  BOOST_CHECK_EQUAL(t.nHoles(), 2);
  BOOST_CHECK_EQUAL(t.nMeasurements(), 3);
  BOOST_CHECK_EQUAL(t.nOutliers(), 2);
  BOOST_CHECK_EQUAL(t.nSharedHits(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
