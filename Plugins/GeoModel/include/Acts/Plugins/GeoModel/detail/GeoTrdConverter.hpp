// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"
#include "Acts/Utilities/Result.hpp"

#include <memory>
#include <tuple>

#include <GeoModelKernel/GeoTrd.h>

class GeoFullPhysVol;
class GeoTrd;

namespace Acts {

class Surface;

namespace detail {
struct GeoTrdConverter {
  /// @brief Convert a GeoTrd to a detector element and surface
  ///
  /// @param geoFPV The full physical volume to convert (contains shape)
  /// @param geoTrd The GeoTrd to convert
  /// @param absTransform from the GeoPhysVol
  /// @param bool sensitive
  ///
  /// @return The detector element and surface
  Result<GeoModelSensitiveSurface> operator()(const PVConstLink& geoPV,
                                              const GeoTrd& geoTrd,
                                              const Transform3& absTransform,
                                              bool sensitive) const;
};
}  // namespace detail

}  // namespace Acts
