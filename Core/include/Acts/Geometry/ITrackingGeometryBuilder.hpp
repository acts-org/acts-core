// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once
#include "Acts/Geometry/GeometryContext.hpp"

#include <memory>

namespace Acts {
class TrackingGeometry;

/// @class ITrackingGeometryBuilder
///
/// Interface class for the TrackingGeometry building,
/// this is used by the TrackingGeometrySvc to build the geometry.
///
/// The TrackingGeometry is written to the detector store and thus not created
/// as a std::shared_ptr.
///
/// The TrackingGeometry is returned as a non-const object in order to recreate
/// from conditions callback if necessary.
///
class ITrackingGeometryBuilder {
 public:
  /// Virtual destructor
  virtual ~ITrackingGeometryBuilder() = default;

  /// TrackingGeometry Interface method
  ///
  /// @param gctx is the geometry context for witch the geometry is built
  ///
  /// @return unique pointer to a newly created TrackingGeometry
  virtual std::unique_ptr<const TrackingGeometry> trackingGeometry(
      const GeometryContext& gctx) const = 0;
};
}  // namespace Acts
