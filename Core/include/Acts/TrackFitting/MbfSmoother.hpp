// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cstddef>
#include <optional>

namespace Acts {

/// Kalman trajectory smoother based on the Modified Bryson–Frazier (mBF)
/// smoother.
///
/// The benefit of the mBF smoother is that it does not require the inverse of
/// the full covariance matrix, but only the inverse of the residual covariance
/// matrix which can be cached by the filter step. The same holds for the
/// Kalman gain matrix.
///
/// This implements not a single smoothing step, but the full backwards
/// smoothing procedure for a filtered, forward trajectory using the stored
/// linearization.
///
/// See
/// [Wikipedia](https://en.wikipedia.org/wiki/Kalman_filter#Modified_Bryson%E2%80%93Frazier_smoother)
/// for more information.
class MbfSmoother {
 public:
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
    (void)logger;

    using TrackStateProxy = typename traj_t::TrackStateProxy;

    TrackStateProxy startTs = trajectory.getTrackState(entryIndex);

    // Notation consistent with the Wikipedia article
    // https://en.wikipedia.org/wiki/Kalman_filter
    BoundMatrix bigLambdaHat = BoundMatrix::Zero();
    BoundVector smallLambdaHat = BoundVector::Zero();

    trajectory.applyBackwards(startTs.index(), [&](TrackStateProxy ts) {
      // ensure the track state has a smoothed component
      ts.addComponents(TrackStatePropMask::Smoothed);

      InternalTrackState internalTrackState(ts);

      // Smoothe the current state
      calculateSmoothed(internalTrackState, bigLambdaHat, smallLambdaHat);

      // We smoothed the last state - no need to update the lambdas
      if (!ts.hasPrevious()) {
        return;
      }

      // Update the lambdas depending on the type of track state
      if (ts.typeFlags().test(TrackStateFlag::MeasurementFlag)) {
        visitMeasurement(internalTrackState, bigLambdaHat, smallLambdaHat);
      } else {
        visitNonMeasurement(internalTrackState, bigLambdaHat, smallLambdaHat);
      }
    });

    return Result<void>::success();
  }

 private:
  /// Internal track state representation for the smoother.
  /// @note This allows us to move parts of the implementation into the .cpp
  struct InternalTrackState final {
    using Projector =
        typename TrackStateTraits<MultiTrajectoryTraits::MeasurementSizeMax,
                                  false>::Projector;
    using Jacobian =
        typename TrackStateTraits<MultiTrajectoryTraits::MeasurementSizeMax,
                                  false>::Covariance;
    using Parameters =
        typename TrackStateTraits<MultiTrajectoryTraits::MeasurementSizeMax,
                                  false>::Parameters;
    using Covariance =
        typename TrackStateTraits<MultiTrajectoryTraits::MeasurementSizeMax,
                                  false>::Covariance;

    struct Measurement final {
      unsigned int calibratedSize{0};
      // This is used to build a covariance matrix view in the .cpp file
      const double* calibrated{nullptr};
      const double* calibratedCovariance{nullptr};
      Projector projector;

      template <typename TrackStateProxy>
      explicit Measurement(TrackStateProxy ts)
          : calibratedSize(ts.calibratedSize()),
            calibrated(ts.effectiveCalibrated().data()),
            calibratedCovariance(ts.effectiveCalibratedCovariance().data()),
            projector(ts.projector()) {}
    };

    Jacobian jacobian;

    Parameters predicted;
    Covariance predictedCovariance;
    Parameters filtered;
    Covariance filteredCovariance;
    Parameters smoothed;
    Covariance smoothedCovariance;

    std::optional<Measurement> measurement;

    template <typename TrackStateProxy>
    explicit InternalTrackState(TrackStateProxy ts)
        : jacobian(ts.jacobian()),
          predicted(ts.predicted()),
          predictedCovariance(ts.predictedCovariance()),
          filtered(ts.filtered()),
          filteredCovariance(ts.filteredCovariance()),
          smoothed(ts.smoothed()),
          smoothedCovariance(ts.smoothedCovariance()),
          measurement(ts.typeFlags().test(TrackStateFlag::MeasurementFlag)
                          ? std::optional<Measurement>(ts)
                          : std::nullopt) {}
  };

  /// Calculate the smoothed parameters and covariance.
  void calculateSmoothed(InternalTrackState& ts,
                         const BoundMatrix& bigLambdaHat,
                         const BoundVector& smallLambdaHat) const;

  /// Visit a non-measurement track state and update the lambdas.
  void visitNonMeasurement(const InternalTrackState& ts,
                           BoundMatrix& bigLambdaHat,
                           BoundVector& smallLambdaHat) const;

  /// Visit a measurement track state and update the lambdas.
  void visitMeasurement(const InternalTrackState& ts, BoundMatrix& bigLambdaHat,
                        BoundVector& smallLambdaHat) const;
};

}  // namespace Acts
