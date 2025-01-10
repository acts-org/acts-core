// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackStateType.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/TrackHelpers.hpp"

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

template <typename TrackContainer>
auto createTestTrackState(TrackContainer& tc) {
  auto t = tc.makeTrack();

  auto ts = t.appendTrackState();

  ts.allocateCalibrated(Vector2::Zero(), SquareMatrix2::Identity());
  ts.setProjectorSubspaceIndices(std::array{eBoundLoc0, eBoundLoc1});

  ts.predicted() = BoundVector::Zero();
  ts.predicted()[eBoundLoc0] = 1.;
  ts.predicted()[eBoundLoc1] = 1.;
  ts.predictedCovariance() = BoundMatrix::Identity() * 1.;

  ts.filtered() = BoundVector::Zero();
  ts.filtered()[eBoundLoc0] = 0.5;
  ts.filtered()[eBoundLoc1] = 0.5;
  ts.filteredCovariance() = BoundMatrix::Identity() * 0.5;

  ts.smoothed() = BoundVector::Zero();
  ts.smoothed()[eBoundLoc0] = 0.1;
  ts.smoothed()[eBoundLoc1] = 0.1;
  ts.smoothedCovariance() = BoundMatrix::Identity() * 0.1;

  return ts;
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
                                   {},
                                   {HoleFlag},
                                   {MeasurementFlag},
                                   {OutlierFlag},
                                   {MeasurementFlag, SharedHitFlag},
                                   {HoleFlag},
                                   {OutlierFlag},
                                   {HoleFlag},
                                   {MeasurementFlag},
                                   {OutlierFlag},
                                   {},
                               });

  calculateTrackQuantities(t);

  BOOST_CHECK_EQUAL(t.nTrackStates(), 11);
  BOOST_CHECK_EQUAL(t.nHoles(), 3);
  BOOST_CHECK_EQUAL(t.nMeasurements(), 3);
  BOOST_CHECK_EQUAL(t.nOutliers(), 3);
  BOOST_CHECK_EQUAL(t.nSharedHits(), 1);

  trimTrackFront(t, true, true, true, true);
  calculateTrackQuantities(t);

  BOOST_CHECK_EQUAL(t.nTrackStates(), 9);
  BOOST_CHECK_EQUAL(t.nHoles(), 2);
  BOOST_CHECK_EQUAL(t.nMeasurements(), 3);
  BOOST_CHECK_EQUAL(t.nOutliers(), 3);
  BOOST_CHECK_EQUAL(t.nSharedHits(), 1);

  trimTrackBack(t, true, true, true, true);
  calculateTrackQuantities(t);

  BOOST_CHECK_EQUAL(t.nTrackStates(), 7);
  BOOST_CHECK_EQUAL(t.nHoles(), 2);
  BOOST_CHECK_EQUAL(t.nMeasurements(), 3);
  BOOST_CHECK_EQUAL(t.nOutliers(), 2);
  BOOST_CHECK_EQUAL(t.nSharedHits(), 1);

  trimTrack(t, true, true, true, true);
  calculateTrackQuantities(t);

  BOOST_CHECK_EQUAL(t.nTrackStates(), 7);
  BOOST_CHECK_EQUAL(t.nHoles(), 2);
  BOOST_CHECK_EQUAL(t.nMeasurements(), 3);
  BOOST_CHECK_EQUAL(t.nOutliers(), 2);
  BOOST_CHECK_EQUAL(t.nSharedHits(), 1);
}

BOOST_AUTO_TEST_CASE(CalculatePredictedChi2) {
  TrackContainer tc{VectorTrackContainer{}, VectorMultiTrajectory{}};
  auto ts = createTestTrackState(tc);

  // reference found by running the code
  BOOST_CHECK_CLOSE(calculatePredictedChi2(ts), 1., 1e-6);
}

BOOST_AUTO_TEST_CASE(CalculateFilteredChi2) {
  TrackContainer tc{VectorTrackContainer{}, VectorMultiTrajectory{}};
  auto ts = createTestTrackState(tc);

  // reference found by running the code
  BOOST_CHECK_CLOSE(calculateFilteredChi2(ts), 1. / 3., 1e-6);
}

BOOST_AUTO_TEST_CASE(CalculateSmoothedChi2) {
  TrackContainer tc{VectorTrackContainer{}, VectorMultiTrajectory{}};
  auto ts = createTestTrackState(tc);

  // reference found by running the code
  BOOST_CHECK_CLOSE(calculateSmoothedChi2(ts), 1. / 55., 1e-6);
}

BOOST_AUTO_TEST_CASE(CalculateUnbiasedParametersCovariance) {
  TrackContainer tc{VectorTrackContainer{}, VectorMultiTrajectory{}};
  auto ts = createTestTrackState(tc);

  auto [params, cov] = calculateUnbiasedParametersCovariance(ts);

  // reference found by running the code
  BoundVector refParams = BoundVector::Zero();
  refParams[eBoundLoc0] = 1. / 9.;
  refParams[eBoundLoc1] = 1. / 9.;
  BoundMatrix refCov = BoundMatrix::Identity() * 0.1;
  refCov(eBoundLoc0, eBoundLoc0) = 1. / 9.;
  refCov(eBoundLoc1, eBoundLoc1) = 1. / 9.;

  CHECK_CLOSE_ABS(params, refParams, 1e-6);
  CHECK_CLOSE_ABS(cov, refCov, 1e-6);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
