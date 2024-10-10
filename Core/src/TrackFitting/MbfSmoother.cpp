// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/TrackFitting/MbfSmoother.hpp"

namespace Acts {

void MbfSmoother::calculateSmoothed(InternalTrackState& ts,
                                    const BoundMatrix& bigLambdaHat,
                                    const BoundVector& smallLambdaHat) const {
  ts.smoothedCovariance = ts.filteredCovariance - ts.filteredCovariance *
                                                      bigLambdaHat *
                                                      ts.filteredCovariance;
  ts.smoothed = ts.filtered - ts.filteredCovariance * smallLambdaHat;
}

void MbfSmoother::visitNonMeasurement(const InternalTrackState& ts,
                                      BoundMatrix& bigLambdaHat,
                                      BoundVector& smallLambdaHat) const {
  const InternalTrackState::Jacobian F = ts.jacobian;

  bigLambdaHat = F.transpose() * bigLambdaHat * F;
  smallLambdaHat = F.transpose() * smallLambdaHat;
}

void MbfSmoother::visitMeasurement(const InternalTrackState& ts,
                                   BoundMatrix& bigLambdaHat,
                                   BoundVector& smallLambdaHat) const {
  assert(ts.measurement.has_value());

  const InternalTrackState::Measurement& measurement = ts.measurement.value();
  const InternalTrackState::Jacobian F = ts.jacobian;

  visit_measurement(measurement.calibratedSize, [&](auto N) -> void {
    constexpr std::size_t kMeasurementSize = decltype(N)::value;

    using MeasurementMatrix =
        Eigen::Matrix<ActsScalar, kMeasurementSize, eBoundSize>;
    using CovarianceMatrix =
        Eigen::Matrix<ActsScalar, kMeasurementSize, kMeasurementSize>;
    using KalmanGainMatrix =
        Eigen::Matrix<ActsScalar, eBoundSize, kMeasurementSize>;

    typename TrackStateTraits<kMeasurementSize, true>::Calibrated calibrated{
        measurement.calibrated};
    typename TrackStateTraits<kMeasurementSize, true>::CalibratedCovariance
        calibratedCovariance{measurement.calibratedCovariance};

    // Measurement matrix
    const MeasurementMatrix H =
        measurement.projector
            .template topLeftCorner<kMeasurementSize, eBoundSize>();

    // Residual covariance
    const CovarianceMatrix S =
        (H * ts.predictedCovariance * H.transpose() + calibratedCovariance);
    // TODO Sinv could be cached by the filter step
    const CovarianceMatrix SInv = S.inverse();

    // Kalman gain
    // TODO K could be cached by the filter step
    const KalmanGainMatrix K = (ts.predictedCovariance * H.transpose() * SInv);

    const Acts::BoundMatrix CHat = (Acts::BoundMatrix::Identity() - K * H);
    const Eigen::Matrix<ActsScalar, kMeasurementSize, 1> y =
        (calibrated - H * ts.predicted);

    const Acts::BoundMatrix bigLambdaTilde =
        (H.transpose() * SInv * H + CHat.transpose() * bigLambdaHat * CHat);
    const Eigen::Matrix<ActsScalar, eBoundSize, 1> smallLambdaTilde =
        (-H.transpose() * SInv * y + CHat.transpose() * smallLambdaHat);

    bigLambdaHat = F.transpose() * bigLambdaTilde * F;
    smallLambdaHat = F.transpose() * smallLambdaTilde;
  });
}

}  // namespace Acts
