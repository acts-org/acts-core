// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"

namespace Acts {
class ConstVectorMultiTrajectory;
class VectorMultiTrajectory;
}  // namespace Acts

namespace ActsExamples {

struct RefittingCalibrator {
  using Proxy = Acts::VectorMultiTrajectory::TrackStateProxy;
  using ConstProxy = Acts::ConstVectorMultiTrajectory::ConstTrackStateProxy;

  struct RefittingSourceLink {
    ConstProxy state;
  };

  static const Acts::Surface* accessSurface(
      const Acts::SourceLink& sourceLink) {
    const auto& refittingSl = sourceLink.get<RefittingSourceLink>();
    return &refittingSl.state.referenceSurface();
  }

  void calibrate(const Acts::GeometryContext& gctx,
                 const Acts::CalibrationContext& cctx,
                 const Acts::SourceLink& sourceLink, Proxy trackState) const;
};

}  // namespace ActsExamples
