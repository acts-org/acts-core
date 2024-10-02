// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/TrackStateType.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/TrackFitting/MbfSmoother.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cmath>
#include <cstddef>

namespace {

using namespace Acts;
using namespace Acts::Test;

using ParametersVector = Acts::BoundVector;
using CovarianceMatrix = Acts::BoundSquareMatrix;
using Jacobian = Acts::BoundMatrix;

const Acts::GeometryContext tgContext;

}  // namespace

BOOST_AUTO_TEST_SUITE(TrackFittingMbfSmoother)

BOOST_AUTO_TEST_CASE(Smooth) {
  VectorMultiTrajectory traj;

  std::array<BoundIndices, 2> projector{eBoundLoc0, eBoundLoc1};

  // Make dummy track parameter
  CovarianceMatrix covTrk;
  covTrk.setIdentity();
  covTrk.diagonal() << 0.08, 0.3, 1, 1, 1, 1;
  BoundVector parValues;

  std::size_t ts_idx = traj.addTrackState(TrackStatePropMask::All);
  auto ts = traj.getTrackState(ts_idx);
  ts.typeFlags().set(TrackStateFlag::MeasurementFlag);

  ts.predicted() << 0.3, 0.5, 0.5 * M_PI, 0., 1 / 100., 0.;
  ts.predictedCovariance() = covTrk;

  ts.allocateCalibrated(2);
  ts.calibrated<2>() << 0.351, 0.473;
  ts.calibratedCovariance<2>() << 1e+8, 0., 0., 1e+8;
  ts.setSubspaceIndices<2>(projector);

  ts.filtered() << 0.301, 0.503, 0.5 * M_PI, 0., 1 / 100., 0.;
  ts.filteredCovariance() = covTrk;
  ts.pathLength() = 1.;
  ts.jacobian().setIdentity();

  ts_idx = traj.addTrackState(TrackStatePropMask::All, ts_idx);
  ts = traj.getTrackState(ts_idx);
  ts.typeFlags().set(TrackStateFlag::MeasurementFlag);

  ts.predicted() << 0.2, 0.5, 0.5 * M_PI, 0., 1 / 100., 0.;
  ts.predictedCovariance() = covTrk;

  ts.allocateCalibrated(2);
  ts.calibrated<2>() << 0.351, 0.473;
  ts.calibratedCovariance<2>() << 1e+8, 0., 0., 1e+8;
  ts.setSubspaceIndices<2>(projector);

  ts.filtered() << 0.27, 0.53, 0.5 * M_PI, 0., 1 / 100., 0.;
  ts.filteredCovariance() = covTrk;
  ts.pathLength() = 2.;
  ts.jacobian().setIdentity();

  ts_idx = traj.addTrackState(TrackStatePropMask::All, ts_idx);
  ts = traj.getTrackState(ts_idx);
  ts.typeFlags().set(TrackStateFlag::MeasurementFlag);

  ts.predicted() << 0.35, 0.49, 0.5 * M_PI, 0., 1 / 100., 0.;
  ts.predictedCovariance() = covTrk;

  ts.allocateCalibrated(2);
  ts.calibrated<2>() << 0.351, 0.473;
  ts.calibratedCovariance<2>() << 1e+8, 0., 0., 1e+8;
  ts.setSubspaceIndices<2>(projector);

  ts.filtered() << 0.33, 0.43, 0.5 * M_PI, 0., 1 / 100., 0.;
  ts.filteredCovariance() = covTrk;
  ts.pathLength() = 3.;
  ts.jacobian().setIdentity();

  // "smooth" these three track states
  BOOST_CHECK(MbfSmoother()(tgContext, traj, ts_idx).ok());

  // Regression tests, only tests very basic correctness of the math, but tests
  // for regressions in the result.

  auto ts1 = traj.getTrackState(0);
  BOOST_CHECK(ts1.hasSmoothed());
  BOOST_CHECK_NE(ts1.filtered(), ts1.smoothed());

  double tol = 1e-6;

  ParametersVector expPars;
  expPars << 0.301, 0.503, 1.5707963, 0.0, 0.01, 0.0;
  CovarianceMatrix expCov;
  expCov.setIdentity();
  expCov.diagonal() << 0.08, 0.3, 1.0, 1.0, 1.0, 1.0;
  CHECK_CLOSE_ABS(ts1.smoothed(), expPars, tol);
  CHECK_CLOSE_ABS(ts1.smoothedCovariance(), expCov, tol);

  auto ts2 = traj.getTrackState(1);
  BOOST_CHECK(ts2.hasSmoothed());
  BOOST_CHECK_NE(ts2.filtered(), ts2.smoothed());

  expPars << 0.27, 0.53, 1.5707963, 0.0, 0.01, 0.0;
  CHECK_CLOSE_ABS(ts2.smoothed(), expPars, tol);
  CHECK_CLOSE_ABS(ts2.smoothedCovariance(), expCov, tol);

  auto ts3 = traj.getTrackState(2);
  BOOST_CHECK(ts3.hasSmoothed());
  // last one, smoothed == filtered
  BOOST_CHECK_EQUAL(ts3.filtered(), ts3.smoothed());

  expPars << 0.33, 0.43, 1.5707963, 0.0, 0.01, 0.0;
  CHECK_CLOSE_ABS(ts3.smoothed(), expPars, tol);
  CHECK_CLOSE_ABS(ts3.smoothedCovariance(), expCov, tol);
}

BOOST_AUTO_TEST_SUITE_END()
