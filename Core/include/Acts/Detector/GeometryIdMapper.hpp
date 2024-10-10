// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Detector/interface/IGeometryIdGenerator.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <map>

namespace Acts::Experimental {

/// @brief This is a mapper of geometry ids, which can be used to
/// assign predefined geometry ids to objects
///
/// @tparam SourceIdentifier is the type of the source identifier
/// @tparam SourceCapture is the type of the source capture function/struct
///
/// The source capture function/struct is a callable object/function that
/// can navigate from the provided surface to the source identifier. Usually
/// this would happen via the associated detector element.
///
/// The only requirement is that the source identifier can be established
/// from the object that receives the target geometry id itself.
template <typename SourceIdentifier, typename SourceCapture>
class GeometryIdMapper final : public IGeometryIdGenerator {
 public:
  /// @brief  Nested config struct
  struct Config {
    /// The source identifier to target geometry Id map
    std::map<SourceIdentifier, GeometryIdentifier> sourceTargetMap;
    /// The source capture function
    SourceCapture sourceCapture = SourceCapture();
  };

  /// @brief Cache object
  struct Cache {
    unsigned int volumeCounter = 0u;
    unsigned int portalCounter = 0u;
    unsigned int surfaceCounter = 0u;
  };

  /// @brief Constructor with config
  ///
  /// @param cfg is the geometry configuration object
  /// @param mlogger is the logging instance
  GeometryIdMapper(const Config& cfg,
                   std::unique_ptr<const Logger> mlogger =
                       getDefaultLogger("GeometryIdMapper", Logging::INFO))
      : m_cfg(cfg), m_logger(std::move(mlogger)) {}

  ~GeometryIdMapper() override = default;

  /// @brief Interface method to generate a geometry id cache
  /// @return a geometry id cache wrapped in a std::any object
  IGeometryIdGenerator::GeoIdCache generateCache() const final {
    return Cache{0u, 0u, 0u};
  }

  /// @brief Method for assigning a geometry id to a detector volume
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param dVolume the detector volume to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& cache,
                        DetectorVolume& dVolume) const final {
    auto& sCache = std::any_cast<Cache&>(cache);
    /// Retrieve the source id for the detector volume
    SourceIdentifier vID = m_cfg.sourceCapture(dVolume);
    auto source = m_cfg.sourceTargetMap.find(vID);
    if (source != m_cfg.sourceTargetMap.end()) {
      dVolume.assignGeometryId(source->second);
      ACTS_VERBOSE("Assigning geometry id " << source->second << " to volume "
                                            << dVolume.name() << " with id "
                                            << vID);
      sCache.volumeCounter++;
    }

    // Portals
    std::for_each(dVolume.portalPtrs().begin(), dVolume.portalPtrs().end(),
                  [&](auto& portal) { assignGeometryId(cache, *portal); });

    // Surfaces
    std::for_each(dVolume.surfacePtrs().begin(), dVolume.surfacePtrs().end(),
                  [&](auto& surface) { assignGeometryId(cache, *surface); });

    // Sub volumes
    std::for_each(dVolume.volumePtrs().begin(), dVolume.volumePtrs().end(),
                  [&](auto& volume) { assignGeometryId(cache, *volume); });
  }

  /// @brief Method for assigning a geometry id to a portal
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param portal the portal to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& cache,
                        Portal& portal) const final {
    auto& sCache = std::any_cast<Cache&>(cache);
    /// Retrieve the source id for the portal
    SourceIdentifier pID = m_cfg.sourceCapture(portal);
    auto source = m_cfg.sourceTargetMap.find(pID);
    if (source != m_cfg.sourceTargetMap.end()) {
      portal.surface().assignGeometryId(source->second);
      ACTS_VERBOSE("Assigning geometry id " << source->second << " to portal "
                                            << " with id " << pID);
      sCache.portalCounter++;
    }
  }

  /// @brief Method for assigning a geometry id to a surface
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param surface the surface to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& cache,
                        Surface& surface) const final {
    auto& sCache = std::any_cast<Cache&>(cache);
    /// Retrieve the source id for the surface
    SourceIdentifier sID = m_cfg.sourceCapture(surface);
    auto source = m_cfg.sourceTargetMap.find(sID);
    if (source != m_cfg.sourceTargetMap.end()) {
      ACTS_VERBOSE("Assigning geometry id " << source->second << " to surface "
                                            << " with id " << sID);
      surface.assignGeometryId(source->second);
      sCache.surfaceCounter++;
    }
  }

 private:
  /// Configuration object
  Config m_cfg;

  /// Private access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Acts::Experimental
