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
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Result.hpp"

class GeoFullPhysVol;
class GeoTube;

namespace Acts::detail {

struct GeoTubeConverter {
  Surface::SurfaceType targetShape = Surface::SurfaceType::Straw;

  /// @brief Convert a GeoTube to a detector element and surface
  ///
  /// @param geoFPV The full physical volume to convert (contains shape)
  /// @param geoTube The GeoTube shape to convert
  /// @param absTransform from the GeoPhysVol
  /// @param bool sensitive
  ///
  /// @return The detector element and surface
  Result<GeoModelSensitiveSurface> operator()(const PVConstLink& geoPV,
                                              const GeoTube& geoTube,
                                              const Transform3& absTransform,
                                              bool sensitive) const;
};

}  // namespace Acts::detail
