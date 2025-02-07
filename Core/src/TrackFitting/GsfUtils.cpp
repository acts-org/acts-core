// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/TrackFitting/detail/GsfUtils.hpp"

#include "Acts/EventData/MeasurementHelpers.hpp"
#include "Acts/EventData/SubspaceHelpers.hpp"
#include "Acts/EventData/Types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace Acts::detail {

using TrackStateTraits =
    TrackStateTraits<MultiTrajectoryTraits::MeasurementSizeMax, true>;

double calculateDeterminant(const double* fullCalibratedCovariance,
                            TrackStateTraits::Covariance predictedCovariance,
                            BoundSubspaceIndices projector,
                            unsigned int calibratedSize) {
  return visit_measurement(calibratedSize, [&](auto N) {
    constexpr std::size_t kMeasurementSize = decltype(N)::value;
    std::span<const std::uint8_t, kMeasurementSize> validSubspaceIndices(
        projector.begin(), projector.begin() + kMeasurementSize);
    FixedBoundSubspaceHelper<kMeasurementSize> subspaceHelper(
        validSubspaceIndices);

    typename Acts::TrackStateTraits<
        kMeasurementSize, true>::CalibratedCovariance calibratedCovariance{
        fullCalibratedCovariance};

    const auto H = subspaceHelper.projector();

    return (H * predictedCovariance * H.transpose() + calibratedCovariance)
        .determinant();
  });
}
}  // namespace Acts::detail
