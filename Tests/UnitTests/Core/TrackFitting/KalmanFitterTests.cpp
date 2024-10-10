// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/GenericCurvilinearTrackParameters.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Tests/CommonHelpers/LineSurfaceStub.hpp"
#include "Acts/TrackFitting/GainMatrixSmoother.hpp"
#include "Acts/TrackFitting/GainMatrixUpdater.hpp"
#include "Acts/TrackFitting/KalmanFitter.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <utility>

#include "FitterTestsCommon.hpp"

namespace {

using namespace Acts;
using namespace Acts::Test;
using namespace Acts::detail::Test;
using namespace Acts::UnitLiterals;

using StraightPropagator =
    Acts::Propagator<Acts::StraightLineStepper, Acts::Navigator>;
using ConstantFieldStepper = Acts::EigenStepper<>;
using ConstantFieldPropagator =
    Acts::Propagator<ConstantFieldStepper, Acts::Navigator>;

using KalmanUpdater = Acts::GainMatrixUpdater;
using KalmanSmoother = Acts::GainMatrixSmoother;
using KalmanFitter =
    Acts::KalmanFitter<ConstantFieldPropagator, VectorMultiTrajectory>;

static const auto pion = Acts::ParticleHypothesis::pion();

KalmanUpdater kfUpdater;
KalmanSmoother kfSmoother;

// Construct initial track parameters.
Acts::CurvilinearTrackParameters makeParameters() {
  // create covariance matrix from reasonable standard deviations
  Acts::BoundVector stddev;
  stddev[Acts::eBoundLoc0] = 100_um;
  stddev[Acts::eBoundLoc1] = 100_um;
  stddev[Acts::eBoundTime] = 25_ns;
  stddev[Acts::eBoundPhi] = 2_degree;
  stddev[Acts::eBoundTheta] = 2_degree;
  stddev[Acts::eBoundQOverP] = 1 / 100_GeV;
  Acts::BoundSquareMatrix cov = stddev.cwiseProduct(stddev).asDiagonal();
  // define a track in the transverse plane along x
  Acts::Vector4 mPos4(-3_m, 0., 0., 42_ns);
  return Acts::CurvilinearTrackParameters(mPos4, 0_degree, 90_degree,
                                          1_e / 1_GeV, cov, pion);
}

// Instantiate the tester
const FitterTester tester;

// reconstruction propagator and fitter
auto kfLogger = getDefaultLogger("KalmanFilter", Logging::INFO);
const auto kfZeroPropagator =
    makeConstantFieldPropagator<ConstantFieldStepper>(tester.geometry, 0_T);
const auto kfZero = KalmanFitter(kfZeroPropagator, std::move(kfLogger));

std::default_random_engine rng(42);

auto makeDefaultKalmanFitterOptions() {
  KalmanFitterExtensions<VectorMultiTrajectory> extensions;
  extensions.calibrator
      .connect<&testSourceLinkCalibrator<VectorMultiTrajectory>>();
  extensions.updater.connect<&KalmanUpdater::operator()<VectorMultiTrajectory>>(
      &kfUpdater);
  extensions.smoother
      .connect<&KalmanSmoother::operator()<VectorMultiTrajectory>>(&kfSmoother);
  extensions.surfaceAccessor.connect<
      &Acts::detail::Test::TestSourceLink::SurfaceAccessor::operator()>(
      &tester.surfaceAccessor);

  return KalmanFitterOptions(
      tester.geoCtx, tester.magCtx, tester.calCtx, extensions,
      PropagatorPlainOptions(tester.geoCtx, tester.magCtx));
}

}  // namespace

BOOST_AUTO_TEST_SUITE(TrackFittingKalmanFitter)

BOOST_AUTO_TEST_CASE(ZeroFieldNoSurfaceForward) {
  auto start = makeParameters();
  auto kfOptions = makeDefaultKalmanFitterOptions();

  bool expected_reversed = false;
  bool expected_smoothed = true;
  tester.test_ZeroFieldNoSurfaceForward(kfZero, kfOptions, start, rng,
                                        expected_reversed, expected_smoothed,
                                        true);
}

BOOST_AUTO_TEST_CASE(ZeroFieldWithSurfaceForward) {
  auto start = makeParameters();
  auto kfOptions = makeDefaultKalmanFitterOptions();

  // regular smoothing
  kfOptions.reversedFiltering = false;
  bool expected_reversed = false;
  bool expected_smoothed = true;
  tester.test_ZeroFieldWithSurfaceForward(kfZero, kfOptions, start, rng,
                                          expected_reversed, expected_smoothed,
                                          true);

  // reverse filtering instead of smoothing
  kfOptions.reversedFiltering = true;
  kfOptions.reversedFilteringCovarianceScaling = 100.0;
  expected_reversed = true;
  expected_smoothed = false;
  tester.test_ZeroFieldWithSurfaceForward(kfZero, kfOptions, start, rng,
                                          expected_reversed, expected_smoothed,
                                          true);
}

BOOST_AUTO_TEST_CASE(ZeroFieldWithSurfaceBackward) {
  auto start = makeParameters();
  auto kfOptions = makeDefaultKalmanFitterOptions();

  // regular smoothing
  kfOptions.reversedFiltering = false;
  bool expected_reversed = false;
  bool expected_smoothed = true;
  tester.test_ZeroFieldWithSurfaceBackward(kfZero, kfOptions, start, rng,
                                           expected_reversed, expected_smoothed,
                                           true);

  // reverse filtering instead of smoothing
  kfOptions.reversedFiltering = true;
  kfOptions.reversedFilteringCovarianceScaling = 100.0;
  expected_reversed = true;
  expected_smoothed = false;
  tester.test_ZeroFieldWithSurfaceBackward(kfZero, kfOptions, start, rng,
                                           expected_reversed, expected_smoothed,
                                           true);
}

BOOST_AUTO_TEST_CASE(ZeroFieldWithSurfaceAtExit) {
  auto start = makeParameters();
  auto kfOptions = makeDefaultKalmanFitterOptions();

  bool expected_reversed = false;
  bool expected_smoothed = true;
  tester.test_ZeroFieldWithSurfaceAtExit(kfZero, kfOptions, start, rng,
                                         expected_reversed, expected_smoothed,
                                         true);
}

BOOST_AUTO_TEST_CASE(ZeroFieldShuffled) {
  auto start = makeParameters();
  auto kfOptions = makeDefaultKalmanFitterOptions();

  bool expected_reversed = false;
  bool expected_smoothed = true;
  tester.test_ZeroFieldShuffled(kfZero, kfOptions, start, rng,
                                expected_reversed, expected_smoothed, true);
}

BOOST_AUTO_TEST_CASE(ZeroFieldWithHole) {
  auto start = makeParameters();
  auto kfOptions = makeDefaultKalmanFitterOptions();

  bool expected_reversed = false;
  bool expected_smoothed = true;
  tester.test_ZeroFieldWithHole(kfZero, kfOptions, start, rng,
                                expected_reversed, expected_smoothed, true);
}

BOOST_AUTO_TEST_CASE(ZeroFieldWithOutliers) {
  auto start = makeParameters();

  // fitter options w/o target surface. outlier distance is set to be below the
  // default outlier distance in the `MeasurementsCreator`
  auto kfOptions = makeDefaultKalmanFitterOptions();

  TestOutlierFinder tof{5_mm};
  kfOptions.extensions.outlierFinder
      .connect<&TestOutlierFinder::operator()<VectorMultiTrajectory>>(&tof);

  bool expected_reversed = false;
  bool expected_smoothed = true;
  tester.test_ZeroFieldWithOutliers(kfZero, kfOptions, start, rng,
                                    expected_reversed, expected_smoothed, true);
}

BOOST_AUTO_TEST_CASE(ZeroFieldWithReverseFiltering) {
  auto start = makeParameters();

  auto test = [&](double threshold, bool reverse, bool expected_reversed,
                  bool expected_smoothed) {
    auto kfOptions = makeDefaultKalmanFitterOptions();

    TestReverseFilteringLogic trfl{threshold};
    kfOptions.extensions.reverseFilteringLogic
        .connect<&TestReverseFilteringLogic::operator()<VectorMultiTrajectory>>(
            &trfl);

    kfOptions.reversedFiltering = reverse;
    kfOptions.reversedFilteringCovarianceScaling = 100.0;

    tester.test_ZeroFieldWithReverseFiltering(kfZero, kfOptions, start, rng,
                                              expected_reversed,
                                              expected_smoothed, true);
  };

  // Track of 1 GeV with a threshold set at 0.1 GeV, reversed filtering should
  // not be used
  test(0.1_GeV, false, false, true);

  // Track of 1 GeV with a threshold set at 10 GeV, reversed filtering should
  // be used
  test(10._GeV, false, true, false);

  // Track of 1 GeV with a threshold set at 10 GeV, reversed filtering should
  // be used
  test(0.1_GeV, true, true, false);
}

// TODO this is not really Kalman fitter specific. is probably better tested
// with a synthetic trajectory.
BOOST_AUTO_TEST_CASE(GlobalCovariance) {
  auto start = makeParameters();
  auto kfOptions = makeDefaultKalmanFitterOptions();

  tester.test_GlobalCovariance(kfZero, kfOptions, start, rng);
}

BOOST_AUTO_TEST_SUITE_END()
