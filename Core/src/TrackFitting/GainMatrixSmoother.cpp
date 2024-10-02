// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/TrackFitting/GainMatrixSmoother.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/detail/CovarianceHelper.hpp"
#include "Acts/TrackFitting/KalmanFitterError.hpp"

#include <algorithm>
#include <ostream>
#include <utility>

namespace Acts {

Result<void> GainMatrixSmoother::calculate(
    void* ts, void* prev_ts, const GetParameters& filtered,
    const GetCovariance& filteredCovariance, const GetParameters& smoothed,
    const GetParameters& predicted, const GetCovariance& predictedCovariance,
    const GetCovariance& smoothedCovariance, const GetCovariance& jacobian,
    const Logger& logger) const {
  ACTS_VERBOSE("Prev. predicted covariance\n"
               << predictedCovariance(prev_ts) << "\n, inverse: \n"
               << predictedCovariance(prev_ts).inverse());

  // Gain smoothing matrix
  // NB: The jacobian stored in a state is the jacobian from previous
  // state to this state in forward propagation
  BoundMatrix G = filteredCovariance(ts) * jacobian(prev_ts).transpose() *
                  predictedCovariance(prev_ts).inverse();

  if (G.hasNaN()) {
    ACTS_VERBOSE("Gain smoothing matrix G has NaNs");

    ACTS_VERBOSE("Filtered covariance:\n" << filteredCovariance(ts));
    ACTS_VERBOSE("Jacobian:\n" << jacobian(prev_ts));
    ACTS_VERBOSE("Predicted covariance:\n" << predictedCovariance(prev_ts));
    ACTS_VERBOSE("Inverse of predicted covariance:\n"
                 << predictedCovariance(prev_ts).inverse());

    ACTS_VERBOSE("Gain smoothing matrix G:\n" << G);

    return KalmanFitterError::SmoothFailed;
  }

  ACTS_VERBOSE("Gain smoothing matrix G:\n" << G);

  ACTS_VERBOSE("Calculate smoothed parameters:");
  ACTS_VERBOSE("Filtered parameters: " << filtered(ts).transpose());
  ACTS_VERBOSE("Prev. smoothed parameters: " << smoothed(prev_ts).transpose());
  ACTS_VERBOSE(
      "Prev. predicted parameters: " << predicted(prev_ts).transpose());

  // Calculate the smoothed parameters
  smoothed(ts) = filtered(ts) + G * (smoothed(prev_ts) - predicted(prev_ts));

  ACTS_VERBOSE("Smoothed parameters are: " << smoothed(ts).transpose());
  ACTS_VERBOSE("Calculate smoothed covariance:");
  ACTS_VERBOSE("Prev. smoothed covariance:\n" << smoothedCovariance(prev_ts));

  // And the smoothed covariance
  smoothedCovariance(ts) =
      filteredCovariance(ts) +
      G * (smoothedCovariance(prev_ts) - predictedCovariance(prev_ts)) *
          G.transpose();

  if (doCovCheckAndAttemptFix) {
    // Check if the covariance matrix is semi-positive definite.
    // If not, make one (could do more) attempt to replace it with the
    // nearest semi-positive def matrix,
    // but it could still be non semi-positive
    BoundSquareMatrix smoothedCov = smoothedCovariance(ts);
    if (!detail::CovarianceHelper<BoundSquareMatrix>::validate(smoothedCov)) {
      ACTS_DEBUG(
          "Smoothed covariance is not positive definite. Could result in "
          "negative covariance!");
    }
    // Reset smoothed covariance
    smoothedCovariance(ts) = smoothedCov;
  }

  ACTS_VERBOSE("Smoothed covariance is: \n" << smoothedCovariance(ts));

  return Result<void>::success();
}

}  // namespace Acts
