// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/EventData/MeasurementCalibration.hpp"

#include "Acts/EventData/SourceLink.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"

#include <cassert>

namespace Acts {
class VectorMultiTrajectory;
}  // namespace Acts

void ActsExamples::PassThroughCalibrator::calibrate(
    const MeasurementContainer& measurements,
    const ClusterContainer* /*clusters*/, const Acts::GeometryContext& /*gctx*/,
    const Acts::CalibrationContext& /*cctx*/,
    const Acts::SourceLink& sourceLink,
    Acts::VectorMultiTrajectory::TrackStateProxy& trackState) const {
  trackState.setUncalibratedSourceLink(Acts::SourceLink{sourceLink});
  const IndexSourceLink& idxSourceLink = sourceLink.get<IndexSourceLink>();

  assert((idxSourceLink.index() < measurements.size()) &&
         "Source link index is outside the container bounds");

  const ConstVariableBoundMeasurementProxy measurement =
      measurements.getMeasurement(idxSourceLink.index());

  Acts::visit_measurement(measurement.size(), [&](auto N) -> void {
    constexpr std::size_t kMeasurementSize = decltype(N)::value;
    const ConstFixedBoundMeasurementProxy<kMeasurementSize> fixedMeasurement =
        static_cast<ConstFixedBoundMeasurementProxy<kMeasurementSize>>(
            measurement);

    trackState.allocateCalibrated(fixedMeasurement.parameters().eval(),
                                  fixedMeasurement.covariance().eval());
    trackState.setProjectorSubspaceIndices(fixedMeasurement.subspaceIndices());
  });
}

ActsExamples::MeasurementCalibratorAdapter::MeasurementCalibratorAdapter(
    const MeasurementCalibrator& calibrator,
    const MeasurementContainer& measurements, const ClusterContainer* clusters)
    : m_calibrator{calibrator},
      m_measurements{measurements},
      m_clusters{clusters} {}

void ActsExamples::MeasurementCalibratorAdapter::calibrate(
    const Acts::GeometryContext& gctx, const Acts::CalibrationContext& cctx,
    const Acts::SourceLink& sourceLink,
    Acts::VectorMultiTrajectory::TrackStateProxy trackState) const {
  return m_calibrator.calibrate(m_measurements, m_clusters, gctx, cctx,
                                sourceLink, trackState);
}
