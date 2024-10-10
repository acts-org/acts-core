// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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

    trackState.allocateCalibrated(Size);
    trackState.template calibrated<Size>() =
        sl.state.template calibrated<Size>();
    trackState.template calibratedCovariance<Size>() =
        sl.state.template calibratedCovariance<Size>();
  });

  trackState.setBoundSubspaceIndices(sl.state.boundSubspaceIndices());
}

}  // namespace ActsExamples
