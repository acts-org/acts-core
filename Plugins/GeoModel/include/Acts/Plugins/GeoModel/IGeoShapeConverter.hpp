// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"
#include "Acts/Utilities/Result.hpp"

#include <memory>
#include <tuple>

class GeoFullPhysVol;

namespace Acts {

class Surface;

/// @class IGeoShapeConverter
///
/// Interface for the conversion of GeoShapes to Acts surfaces or volumes
class IGeoShapeConverter {
 public:
  /// Virtual destructor
  virtual ~IGeoShapeConverter() = default;

  /// @brief Convert a GeoShape to a detector element and surface
  ///
  /// @param geoFPV The full physical volume to convert (contains shape)
  ///
  /// @return The detector element and surface
  virtual Result<GeoModelSensitiveSurface> toSensitiveSurface(
      PVConstLink geoPV, const Transform3& transform) const = 0;

  /// @brief Convert a GeoShape to a detector element and passive surface
  ///
  /// @param geoFPV The full physical volume to convert (contains shape)
  ///
  /// @return The representing surface
  virtual Result<std::shared_ptr<Surface>> toPassiveSurface(
      PVConstLink geoPV, const Transform3& transform) const = 0;
};

}  // namespace Acts
