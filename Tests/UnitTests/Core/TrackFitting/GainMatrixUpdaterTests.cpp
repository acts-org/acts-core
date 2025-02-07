// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/TrackFitting/GainMatrixUpdater.hpp"
#include "Acts/TrackFitting/KalmanFitterError.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"
#include "Acts/Utilities/Result.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace {

using namespace Acts;
using namespace Acts::Test;
using namespace Acts::detail::Test;

using ParametersVector = Acts::BoundVector;
using CovarianceMatrix = Acts::BoundSquareMatrix;
using Jacobian = Acts::BoundMatrix;

constexpr double tol = 1e-6;
const Acts::GeometryContext tgContext;

}  // namespace

BOOST_AUTO_TEST_SUITE(TrackFittingGainMatrixUpdater)

BOOST_AUTO_TEST_CASE(Update) {
  // Make dummy measurement
  Vector2 measPar(-0.1, 0.45);
  SquareMatrix2 measCov = Vector2(0.04, 0.1).asDiagonal();
  auto sourceLink = TestSourceLink(eBoundLoc0, eBoundLoc1, measPar, measCov);

  // Make dummy track parameters
  ParametersVector trkPar;
  trkPar << 0.3, 0.5, std::numbers::pi / 2., 0.3 * std::numbers::pi, 0.01, 0.;
  CovarianceMatrix trkCov = CovarianceMatrix::Zero();
  trkCov.diagonal() << 0.08, 0.3, 1, 1, 1, 0;

  // Make trajectory w/ one state
  VectorMultiTrajectory traj;
  auto idx = traj.addTrackState(TrackStatePropMask::All);
  auto ts = traj.getTrackState(idx);

  // Fill the state w/ the dummy information
  ts.predicted() = trkPar;
  ts.predictedCovariance() = trkCov;
  ts.pathLength() = 0.;
  BOOST_CHECK(!ts.hasUncalibratedSourceLink());
  testSourceLinkCalibrator<VectorMultiTrajectory>(
      tgContext, CalibrationContext{}, SourceLink{std::move(sourceLink)}, ts);
  BOOST_CHECK(ts.hasUncalibratedSourceLink());

  // Check that the state has storage available
  BOOST_CHECK(ts.hasPredicted());
  BOOST_CHECK(ts.hasFiltered());
  BOOST_CHECK(ts.hasCalibrated());

  // Gain matrix update and filtered state
  BOOST_CHECK(GainMatrixUpdater()
                  .operator()<VectorMultiTrajectory>(tgContext, ts)
                  .ok());

  // Check for regression. This does NOT test if the math is correct, just that
  // the result is the same as when the test was written.

  ParametersVector expPar;
  expPar << 0.0333333, 0.4625000, 1.5707963, 0.9424778, 0.0100000, 0.0000000;
  CHECK_CLOSE_ABS(ts.filtered(), expPar, tol);

  CovarianceMatrix expCov = CovarianceMatrix::Zero();
  expCov.diagonal() << 0.0266667, 0.0750000, 1.0000000, 1.0000000, 1.0000000,
      0.0000000;
  CHECK_CLOSE_ABS(ts.filteredCovariance(), expCov, tol);

  CHECK_CLOSE_ABS(ts.chi2(), 1.33958, 1e-4);
}

BOOST_AUTO_TEST_CASE(UpdateFailed) {
  // Make dummy measurement
  Vector2 measPar(-0.1, 0.45);
  SquareMatrix2 measCov = SquareMatrix2::Zero();
  auto sourceLink = TestSourceLink(eBoundLoc0, eBoundLoc1, measPar, measCov);

  // Make dummy track parameters
  ParametersVector trkPar;
  trkPar << 0.3, 0.5, std::numbers::pi / 2., 0.3 * std::numbers::pi, 0.01, 0.;
  CovarianceMatrix trkCov = CovarianceMatrix::Zero();

  // Make trajectory w/ one state
  VectorMultiTrajectory traj;
  auto idx = traj.addTrackState(TrackStatePropMask::All);
  auto ts = traj.getTrackState(idx);

  // Fill the state w/ the dummy information
  ts.predicted() = trkPar;
  ts.predictedCovariance() = trkCov;
  ts.pathLength() = 0.;
  BOOST_CHECK(!ts.hasUncalibratedSourceLink());
  testSourceLinkCalibrator<VectorMultiTrajectory>(
      tgContext, CalibrationContext{}, SourceLink{std::move(sourceLink)}, ts);
  BOOST_CHECK(ts.hasUncalibratedSourceLink());

  // Check that the state has storage available
  BOOST_CHECK(ts.hasPredicted());
  BOOST_CHECK(ts.hasFiltered());
  BOOST_CHECK(ts.hasCalibrated());

  // Gain matrix update and filtered state
  BOOST_CHECK(GainMatrixUpdater()
                  .operator()<VectorMultiTrajectory>(tgContext, ts)
                  .error() == KalmanFitterError::UpdateFailed);
}

BOOST_AUTO_TEST_SUITE_END()
