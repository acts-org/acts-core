// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/TrackFitting/GainMatrixSmoother.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cmath>
#include <cstddef>
#include <numbers>

namespace {

using namespace Acts;
using namespace Acts::Test;

using ParametersVector = Acts::BoundVector;
using CovarianceMatrix = Acts::BoundSquareMatrix;
using Jacobian = Acts::BoundMatrix;

const Acts::GeometryContext tgContext;

}  // namespace

BOOST_AUTO_TEST_SUITE(TrackFittingGainMatrixSmoother)

BOOST_AUTO_TEST_CASE(Smooth) {
  VectorMultiTrajectory traj;
  std::size_t ts_idx = traj.addTrackState(TrackStatePropMask::All);
  auto ts = traj.getTrackState(ts_idx);

  // Make dummy track parameter
  CovarianceMatrix covTrk;
  covTrk.setIdentity();
  covTrk.diagonal() << 0.08, 0.3, 1, 1, 1, 1;
  BoundVector parValues;
  parValues << 0.3, 0.5, std::numbers::pi / 2., 0., 1 / 100., 0.;

  ts.predicted() = parValues;
  ts.predictedCovariance() = covTrk;

  parValues << 0.301, 0.503, std::numbers::pi / 2., 0., 1 / 100., 0.;

  ts.filtered() = parValues;
  ts.filteredCovariance() = covTrk;
  ts.pathLength() = 1.;
  ts.jacobian().setIdentity();

  ts_idx = traj.addTrackState(TrackStatePropMask::All, ts_idx);
  ts = traj.getTrackState(ts_idx);

  parValues << 0.2, 0.5, std::numbers::pi / 2., 0., 1 / 100., 0.;
  ts.predicted() = parValues;
  ts.predictedCovariance() = covTrk;

  parValues << 0.27, 0.53, std::numbers::pi / 2., 0., 1 / 100., 0.;
  ts.filtered() = parValues;
  ts.filteredCovariance() = covTrk;
  ts.pathLength() = 2.;
  ts.jacobian().setIdentity();

  ts_idx = traj.addTrackState(TrackStatePropMask::All, ts_idx);
  ts = traj.getTrackState(ts_idx);

  parValues << 0.35, 0.49, std::numbers::pi / 2., 0., 1 / 100., 0.;
  ts.predicted() = parValues;
  ts.predictedCovariance() = covTrk;

  parValues << 0.33, 0.43, std::numbers::pi / 2., 0., 1 / 100., 0.;
  ts.filtered() = parValues;
  ts.filteredCovariance() = covTrk;
  ts.pathLength() = 3.;
  ts.jacobian().setIdentity();

  // "smooth" these three track states
  BOOST_CHECK(GainMatrixSmoother()(tgContext, traj, ts_idx).ok());

  // Regression tests, only tests very basic correctness of the math, but tests
  // for regressions in the result.

  auto ts1 = traj.getTrackState(0);
  BOOST_CHECK(ts1.hasSmoothed());
  BOOST_CHECK_NE(ts1.filtered(), ts1.smoothed());

  double tol = 1e-6;

  ParametersVector expPars;
  expPars << 0.3510000, 0.4730000, 1.5707963, 0.0000000, 0.0100000, 0.0000000;
  CovarianceMatrix expCov;
  expCov.setIdentity();
  expCov.diagonal() << 0.0800000, 0.3000000, 1.0000000, 1.0000000, 1.0000000,
      1.0000000;
  CHECK_CLOSE_ABS(ts1.smoothed(), expPars, tol);
  CHECK_CLOSE_ABS(ts1.smoothedCovariance(), expCov, tol);

  auto ts2 = traj.getTrackState(1);
  BOOST_CHECK(ts2.hasSmoothed());
  BOOST_CHECK_NE(ts2.filtered(), ts2.smoothed());

  expPars << 0.2500000, 0.4700000, 1.5707963, 0.0000000, 0.0100000, 0.0000000;
  CHECK_CLOSE_ABS(ts2.smoothed(), expPars, tol);
  CHECK_CLOSE_ABS(ts2.smoothedCovariance(), expCov, tol);

  auto ts3 = traj.getTrackState(2);
  BOOST_CHECK(ts3.hasSmoothed());
  // last one, smoothed == filtered
  BOOST_CHECK_EQUAL(ts3.filtered(), ts3.smoothed());

  expPars << 0.3300000, 0.4300000, 1.5707963, 0.0000000, 0.0100000, 0.0000000;
  CHECK_CLOSE_ABS(ts3.smoothed(), expPars, tol);
  CHECK_CLOSE_ABS(ts3.smoothedCovariance(), expCov, tol);
}

BOOST_AUTO_TEST_SUITE_END()
