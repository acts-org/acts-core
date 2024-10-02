// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/TrackFitting/detail/GsfUtils.hpp"

#include "Acts/EventData/MeasurementHelpers.hpp"

#include <cstddef>

namespace Acts::detail {

using TrackStateTraits =
    TrackStateTraits<MultiTrajectoryTraits::MeasurementSizeMax, true>;

ActsScalar calculateDeterminant(
    const double* fullCalibratedCovariance,
    TrackStateTraits::Covariance predictedCovariance,
    TrackStateTraits::Projector projector, unsigned int calibratedSize) {
  return visit_measurement(calibratedSize, [&](auto N) {
    constexpr std::size_t kMeasurementSize = decltype(N)::value;

    typename Acts::TrackStateTraits<
        kMeasurementSize, true>::CalibratedCovariance calibratedCovariance{
        fullCalibratedCovariance};

    const auto H =
        projector.template topLeftCorner<kMeasurementSize, eBoundSize>().eval();

    return (H * predictedCovariance * H.transpose() + calibratedCovariance)
        .determinant();
  });
}
}  // namespace Acts::detail
