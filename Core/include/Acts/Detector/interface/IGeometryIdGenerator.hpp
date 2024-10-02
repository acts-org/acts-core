// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <any>

namespace Acts {

class Surface;

namespace Experimental {

class Portal;
class DetectorVolume;

/// @brief This is the interface for generating geometry ids and assign them
/// to detector volumes, portals and surfaces
///
class IGeometryIdGenerator {
 public:
  using GeoIdCache = std::any;

  virtual ~IGeometryIdGenerator() = default;

  /// @brief  Virtual interface method to generate a geometry id cache
  /// @return a geometry id cache wrapped in a std::any object
  virtual GeoIdCache generateCache() const = 0;

  /// The virtual interface definition for assigning a geometry id to
  /// a detector volume
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param dVolume the detector volume to assign the geometry id to
  virtual void assignGeometryId(GeoIdCache& cache,
                                DetectorVolume& dVolume) const = 0;

  /// The virtual interface definition for assigning a geometry id to
  /// a portal
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param portal the portal to assign the geometry id to
  virtual void assignGeometryId(GeoIdCache& cache, Portal& portal) const = 0;

  /// @brief  The virtual interface definition for assigning a geometry id to
  /// a surface
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param surface the surface to assign the geometry id to
  virtual void assignGeometryId(GeoIdCache& cache, Surface& surface) const = 0;
};

}  // namespace Experimental
}  // namespace Acts
