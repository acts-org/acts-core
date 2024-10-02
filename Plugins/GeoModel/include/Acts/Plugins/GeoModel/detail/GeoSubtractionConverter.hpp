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

#include <GeoModelKernel/GeoShapeSubtraction.h>

namespace Acts::detail {
struct GeoSubtractionConverter {
  /// @brief Convert a GeoBox to a detector element and surface
  ///
  /// @param geoPV dummy to be compatible with other converters
  /// @param geoSub The GeoShapeSubtraction to convert
  /// @param absTransform from the GeoPhysVol
  /// @param dummy to be compatible with other converters
  ///
  /// @return The detector element and surface
  Result<GeoModelSensitiveSurface> operator()(
      [[maybe_unused]] const PVConstLink& geoPV,
      const GeoShapeSubtraction& geoSub, const Transform3& absTransform,
      [[maybe_unused]] bool sensitive) const;
};
}  // namespace Acts::detail
