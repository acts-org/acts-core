// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/TrackFitting/KalmanFitterError.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cassert>
#include <cstddef>
#include <system_error>

namespace Acts {

/// Kalman trajectory smoother based on gain matrix formalism.
///
/// This implements not a single smoothing step, but the full backwards
/// smoothing procedure for a filtered, forward trajectory using the stored
/// linearization.
class GainMatrixSmoother {
 public:
  /// Whether to check the covariance matrices if they are semi-positive and if
  /// not attempt to correct them.
  bool doCovCheckAndAttemptFix = false;

  /// Run the Kalman smoothing for one trajectory.
  ///
  /// @param[in] gctx The geometry context to be used
  /// @param[in,out] trajectory The trajectory to be smoothed
  /// @param[in] entryIndex The index of state to start the smoothing
  /// @param[in] logger Where to write logging information to
  template <typename traj_t>
  Result<void> operator()(const GeometryContext& gctx, traj_t& trajectory,
                          std::size_t entryIndex,
                          const Logger& logger = getDummyLogger()) const {
    (void)gctx;

    using TrackStateProxy = typename traj_t::TrackStateProxy;

    GetParameters filtered;
    GetCovariance filteredCovariance;
    GetParameters smoothed;
    GetParameters predicted;
    GetCovariance predictedCovariance;
    GetCovariance smoothedCovariance;
    GetCovariance jacobian;

    filtered.connect([](const void*, void* ts) {
      return static_cast<TrackStateProxy*>(ts)->filtered();
    });
    filteredCovariance.connect([](const void*, void* ts) {
      return static_cast<TrackStateProxy*>(ts)->filteredCovariance();
    });

    smoothed.connect([](const void*, void* ts) {
      return static_cast<TrackStateProxy*>(ts)->smoothed();
    });
    smoothedCovariance.connect([](const void*, void* ts) {
      return static_cast<TrackStateProxy*>(ts)->smoothedCovariance();
    });

    predicted.connect([](const void*, void* ts) {
      return static_cast<TrackStateProxy*>(ts)->predicted();
    });
    predictedCovariance.connect([](const void*, void* ts) {
      return static_cast<TrackStateProxy*>(ts)->predictedCovariance();
    });

    jacobian.connect([](const void*, void* ts) {
      return static_cast<TrackStateProxy*>(ts)->jacobian();
    });

    ACTS_VERBOSE("Invoked GainMatrixSmoother on entry index: " << entryIndex);

    // For the last state: smoothed is filtered - also: switch to next
    ACTS_VERBOSE("Getting previous track state");
    auto prev_ts = trajectory.getTrackState(entryIndex);

    prev_ts.shareFrom(TrackStatePropMask::Filtered,
                      TrackStatePropMask::Smoothed);

    // make sure there is more than one track state
    if (!prev_ts.hasPrevious()) {
      ACTS_VERBOSE("Only one track state given, smoothing terminates early");
      return Result<void>::success();
    }

    ACTS_VERBOSE("Start smoothing from previous track state at index: "
                 << prev_ts.previous());

    // default-constructed error represents success, i.e. an invalid error code
    std::error_code error;
    trajectory.applyBackwards(prev_ts.previous(), [&, this](auto ts) {
      // should have filtered and predicted, this should also include the
      // covariances.
      assert(ts.hasFiltered());
      assert(ts.hasPredicted());

      // previous trackstate should have smoothed and predicted
      assert(prev_ts.hasSmoothed());
      assert(prev_ts.hasPredicted());
      assert(prev_ts.hasJacobian());

      ACTS_VERBOSE("Calculate smoothing matrix:");
      ACTS_VERBOSE("Filtered covariance:\n" << ts.filteredCovariance());
      ACTS_VERBOSE("Jacobian:\n" << prev_ts.jacobian());

      // ensure the track state has a smoothed component
      ts.addComponents(TrackStatePropMask::Smoothed);

      if (auto res = calculate(&ts, &prev_ts, filtered, filteredCovariance,
                               smoothed, predicted, predictedCovariance,
                               smoothedCovariance, jacobian, logger);
          !res.ok()) {
        error = res.error();
        return false;
      }

      prev_ts = ts;
      return true;  // continue execution
    });

    return error ? Result<void>::failure(error) : Result<void>::success();
  }

  using GetParameters =
      Acts::Delegate<TrackStateTraits<MultiTrajectoryTraits::MeasurementSizeMax,
                                      false>::Parameters(void*)>;
  using GetCovariance =
      Acts::Delegate<TrackStateTraits<MultiTrajectoryTraits::MeasurementSizeMax,
                                      false>::Covariance(void*)>;

  Result<void> calculate(void* ts, void* prev_ts, const GetParameters& filtered,
                         const GetCovariance& filteredCovariance,
                         const GetParameters& smoothed,
                         const GetParameters& predicted,
                         const GetCovariance& predictedCovariance,
                         const GetCovariance& smoothedCovariance,
                         const GetCovariance& jacobian,
                         const Logger& logger) const;
};

}  // namespace Acts
