// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/TrackFitting/MbfSmoother.hpp"

#include "Acts/EventData/TrackParameterHelpers.hpp"

#include <cstdint>

namespace Acts {

void MbfSmoother::calculateSmoothed(InternalTrackState& ts,
                                    const BoundMatrix& bigLambdaHat,
                                    const BoundVector& smallLambdaHat) const {
  ts.smoothedCovariance = ts.filteredCovariance - ts.filteredCovariance *
                                                      bigLambdaHat *
                                                      ts.filteredCovariance;
  ts.smoothed = ts.filtered - ts.filteredCovariance * smallLambdaHat;
  // Normalize phi and theta
  ts.smoothed = normalizeBoundParameters(ts.smoothed);
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
    std::span<const std::uint8_t, kMeasurementSize> validSubspaceIndices(
        measurement.projector.begin(),
        measurement.projector.begin() + kMeasurementSize);
    FixedBoundSubspaceHelper<kMeasurementSize> subspaceHelper(
        validSubspaceIndices);

    using ProjectorMatrix = Eigen::Matrix<double, kMeasurementSize, eBoundSize>;
    using CovarianceMatrix =
        Eigen::Matrix<double, kMeasurementSize, kMeasurementSize>;
    using KalmanGainMatrix =
        Eigen::Matrix<double, eBoundSize, kMeasurementSize>;

    typename TrackStateTraits<kMeasurementSize, true>::Calibrated calibrated{
        measurement.calibrated};
    typename TrackStateTraits<kMeasurementSize, true>::CalibratedCovariance
        calibratedCovariance{measurement.calibratedCovariance};

    // Projector matrix
    const ProjectorMatrix H = subspaceHelper.projector();

    // Residual covariance
    const CovarianceMatrix S =
        (H * ts.predictedCovariance * H.transpose() + calibratedCovariance);
    // TODO Sinv could be cached by the filter step
    const CovarianceMatrix SInv = S.inverse();

    // Kalman gain
    // TODO K could be cached by the filter step
    const KalmanGainMatrix K = (ts.predictedCovariance * H.transpose() * SInv);

    const Acts::BoundMatrix CHat = (Acts::BoundMatrix::Identity() - K * H);
    const Eigen::Matrix<double, kMeasurementSize, 1> y =
        (calibrated - H * ts.predicted);

    const Acts::BoundMatrix bigLambdaTilde =
        (H.transpose() * SInv * H + CHat.transpose() * bigLambdaHat * CHat);
    const Eigen::Matrix<double, eBoundSize, 1> smallLambdaTilde =
        (-H.transpose() * SInv * y + CHat.transpose() * smallLambdaHat);

    bigLambdaHat = F.transpose() * bigLambdaTilde * F;
    smallLambdaHat = F.transpose() * smallLambdaTilde;
  });
}

}  // namespace Acts
