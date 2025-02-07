// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
