// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"

namespace Acts {
struct SpacePointBuilderConfig {
  /// Tracking geometry
  std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry;
  /// Perform the perpendicular projection for space point finding
  bool usePerpProj = false;

  /// The accessor to retrieve surfaces from source links
  SourceLinkSurfaceAccessor slSurfaceAccessor;

  SpacePointBuilderConfig() = default;
};

}  // namespace Acts
