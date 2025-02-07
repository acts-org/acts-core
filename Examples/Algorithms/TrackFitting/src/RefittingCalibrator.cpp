// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TrackFitting/RefittingCalibrator.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/MeasurementHelpers.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"

namespace ActsExamples {

void RefittingCalibrator::calibrate(const Acts::GeometryContext& /*gctx*/,
                                    const Acts::CalibrationContext& /*cctx*/,
                                    const Acts::SourceLink& sourceLink,
                                    Proxy trackState) const {
  const auto sl = sourceLink.get<RefittingSourceLink>();

  // Reset the original uncalibrated source link on this track state
  trackState.setUncalibratedSourceLink(sl.state.getUncalibratedSourceLink());

  // Here we construct a measurement by extracting the information available
  // in the state
  Acts::visit_measurement(sl.state.calibratedSize(), [&](auto N) {
    using namespace Acts;
    constexpr int Size = decltype(N)::value;

    trackState.allocateCalibrated(
        sl.state.template calibrated<Size>().eval(),
        sl.state.template calibratedCovariance<Size>().eval());
  });

  trackState.setProjectorSubspaceIndices(sl.state.projectorSubspaceIndices());
}

}  // namespace ActsExamples
