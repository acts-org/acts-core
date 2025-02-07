// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Result.hpp"

#include <memory>
#include <tuple>

#include <GeoModelKernel/GeoShapeUnion.h>

class GeoFullPhysVol;
class GeoTube;

namespace Acts::detail {

struct GeoUnionDoubleTrdConverter {
  /// Merge trapezoids up to this gap
  double gapTolerance = 0.2;

  /// @brief Convert a GeoTube to a detector element and surface
  ///
  /// @param geoFPV The full physical volume to convert (contains shape)
  /// @param geoUnion The GeoUnion shape to convert
  /// @param absTransform from the GeoPhysVol
  /// @param bool sensitive
  ///
  /// @return The detector element and surface
  Result<GeoModelSensitiveSurface> operator()(const PVConstLink& geoPV,
                                              const GeoShapeUnion& geoUnion,
                                              const Transform3& absTransform,
                                              bool sensitive) const;
};

}  // namespace Acts::detail
