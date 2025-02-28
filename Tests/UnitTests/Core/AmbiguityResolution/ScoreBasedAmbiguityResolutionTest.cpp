// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/AmbiguityResolution/ScoreBasedAmbiguityResolution.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/EventData/detail/TestTrackState.hpp"
#include "Acts/Utilities/TrackHelpers.hpp"

#include <map>

using Acts::MultiTrajectoryTraits::IndexType;

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(ScoreBasedAmbiguityResolutionTest)

// Test fixture for ScoreBasedAmbiguityResolution
struct Fixture {
  ScoreBasedAmbiguityResolution::Config config;

  using TrackFeatures = ScoreBasedAmbiguityResolution::TrackFeatures;
  using DetectorConfig = ScoreBasedAmbiguityResolution::DetectorConfig;

  Fixture() {
    // Set up any resources used by the tests
    config.volumeMap = {{8, 0},  {9, 0},  {10, 0}, {13, 0}, {14, 0},
                        {15, 0}, {16, 0}, {17, 0}, {18, 0}, {19, 0},
                        {20, 0}, {22, 1}, {23, 1}, {24, 1}};

    auto tempDetector = DetectorConfig();
    std::vector<DetectorConfig> detectorConfigs = {tempDetector, tempDetector};
    config.detectorConfigs = detectorConfigs;

    config.minScore = 0;
    config.minScoreSharedTracks = 100;
    config.maxShared = 5;
    config.minUnshared = 3;
    config.maxSharedTracksPerMeasurement = 10;
    config.useAmbiguityScoring = false;
  }

  ~Fixture() = default;
};

template <typename TrackContainer, typename FlagsPerState>
auto createTestTrack(TrackContainer& tc, const FlagsPerState& flagsPerState) {
  auto t = tc.makeTrack();
  for (const auto& flags : flagsPerState) {
    auto ts = t.appendTrackState();
    for (auto f : flags) {
      ts.typeFlags().set(f);
    }
  }

  calculateTrackQuantities(t);

  return t;
}

BOOST_FIXTURE_TEST_CASE(ComputeInitialStateTest, Fixture) {
  Fixture fixture;
  // Create an instance of ScoreBasedAmbiguityResolution
  ScoreBasedAmbiguityResolution tester(fixture.config);

  VectorTrackContainer mutVtc;
  VectorMultiTrajectory mutMtj;

  TrackContainer mutTc{mutVtc, mutMtj};
  static_assert(!mutTc.ReadOnly, "Unexpectedly read only");

  auto t = createTestTrack(mutTc, std::vector<std::vector<TrackStateFlag>>{
                                      {MeasurementFlag},
                                      {OutlierFlag},
                                      {MeasurementFlag, SharedHitFlag},
                                      {HoleFlag},
                                      {OutlierFlag},
                                      {HoleFlag},
                                      {MeasurementFlag, SharedHitFlag},
                                      {OutlierFlag},
                                  });

  BOOST_CHECK_EQUAL(t.nHoles(), 2);
  BOOST_CHECK_EQUAL(t.nMeasurements(), 3);
  BOOST_CHECK_EQUAL(t.nOutliers(), 3);
  BOOST_CHECK_EQUAL(t.nSharedHits(), 2);

  ConstVectorTrackContainer vtc{std::move(mutVtc)};
  ConstVectorMultiTrajectory mtj{std::move(mutMtj)};

  TrackContainer ctc{vtc, mtj};

  std::vector<std::vector<TrackFeatures>> trackFeaturesVectors;
  trackFeaturesVectors = tester.computeInitialState(ctc);

  BOOST_CHECK_EQUAL(trackFeaturesVectors.size(), ctc.size());

  std::vector<double> trackScores;
  trackScores = tester.simpleScore(ctc, trackFeaturesVectors);

  BOOST_CHECK_EQUAL(trackScores.size(), ctc.size());

  trackScores = tester.ambiguityScore(ctc, trackFeaturesVectors);

  BOOST_CHECK_EQUAL(trackScores.size(), ctc.size());

  // Assert the expected results
}

BOOST_FIXTURE_TEST_CASE(GetCleanedOutTracksTest, Fixture) {
  Fixture fixture;
  // Create an instance of ScoreBasedAmbiguityResolution
  ScoreBasedAmbiguityResolution tester(fixture.config);

  VectorTrackContainer mutVtc;
  VectorMultiTrajectory mutMtj;

  TrackContainer mutTc{mutVtc, mutMtj};
  static_assert(!mutTc.ReadOnly, "Unexpectedly read only");

  auto t = createTestTrack(mutTc, std::vector<std::vector<TrackStateFlag>>{
                                      {MeasurementFlag},
                                      {OutlierFlag},
                                      {MeasurementFlag, SharedHitFlag},
                                      {HoleFlag},
                                      {OutlierFlag},
                                      {HoleFlag},
                                      {MeasurementFlag, SharedHitFlag},
                                      {OutlierFlag},
                                  });

  BOOST_CHECK_EQUAL(t.nHoles(), 2);
  BOOST_CHECK_EQUAL(t.nMeasurements(), 3);
  BOOST_CHECK_EQUAL(t.nOutliers(), 3);
  BOOST_CHECK_EQUAL(t.nSharedHits(), 2);

  ConstVectorTrackContainer vtc{std::move(mutVtc)};
  ConstVectorMultiTrajectory mtj{std::move(mutMtj)};

  TrackContainer ctc{vtc, mtj};

  std::vector<std::vector<TrackFeatures>> trackFeaturesVectors;
  std::vector<std::vector<std::size_t>> measurementsPerTrackVector;
  std::map<std::size_t, std::size_t> nTracksPerMeasurement;

  trackFeaturesVectors = tester.computeInitialState(ctc);

  BOOST_CHECK_EQUAL(trackFeaturesVectors.size(), ctc.size());

  for (std::size_t iMeasurement = 1; const auto& track : ctc) {
    std::vector<std::size_t> measurementsPerTrack;
    for (auto ts : track.trackStatesReversed()) {
      if (ts.typeFlags().test(Acts::TrackStateFlag::OutlierFlag) ||
          ts.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
        measurementsPerTrack.push_back(iMeasurement);
        if (nTracksPerMeasurement.find(iMeasurement) ==
            nTracksPerMeasurement.end()) {
          nTracksPerMeasurement[iMeasurement] = 0;
        }
        nTracksPerMeasurement[iMeasurement]++;
      }

      iMeasurement++;
    }
    measurementsPerTrackVector.push_back(std::move(measurementsPerTrack));
  }

  auto trackScore = tester.ambiguityScore(ctc, trackFeaturesVectors);

  const auto& track = ctc.getTrack(0);
  std::size_t iTrack = track.index();
  bool accepted = tester.getCleanedOutTracks(track, trackScore[iTrack],
                                             measurementsPerTrackVector[iTrack],
                                             nTracksPerMeasurement);

  BOOST_CHECK_EQUAL(accepted, false);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
