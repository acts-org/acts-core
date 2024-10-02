// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/interface/IGeometryIdGenerator.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <any>

namespace Acts {

class Surface;

namespace Experimental {

class Portal;
class DetectorVolume;

/// @brief This is the default implementation of the geometry id generator
///
/// It is a simple counter based generator that assigns ids to the geometry
/// and increments the counter for each object type.
///
/// Sub counters, i.e. for the sensitive surfaces, are reset after the volume
/// call, such that a new volume or layer volume would start from counter 0
/// again.
///
/// If the generator is configured to override existing ids, it will do so
/// and not respect previously assigned ids.
///
/// If the generator is configured in container mode, it will increase the
/// layer id for each layer volume with confined surfaces.
///
class GeometryIdGenerator final : public IGeometryIdGenerator {
 public:
  /// @brief  Nested config struct
  struct Config {
    /// Container mode
    bool containerMode = false;
    /// Container id (if container mode), will not be incremented
    unsigned int containerId = 0u;
    /// Resetting mode
    bool resetSubCounters = true;
    /// Force override existing ids
    bool overrideExistingIds = false;
  };

  /// @brief Nested cache struct
  struct Cache {
    /// Cache count of the volume, for non-container mode
    unsigned int volumeCount = 0u;
    /// Cache count of the layer volume, for container mode
    unsigned int layerCount = 0u;
    /// Cache count of the portal surfaces
    unsigned int portalCount = 0u;
    /// Cache count of passive surfaces
    unsigned int passiveCount = 0u;
    /// Cache count of sensitive surfaces
    unsigned int sensitiveCount = 0u;
  };

  /// @brief Constructor with config
  ///
  /// @param cfg is the geometry configuration object
  /// @param mlogger is the logging instance
  GeometryIdGenerator(const Config& cfg,
                      std::unique_ptr<const Logger> mlogger = getDefaultLogger(
                          "GeometryIdGenerator", Logging::INFO))
      : m_cfg(cfg), m_logger(std::move(mlogger)) {}

  ~GeometryIdGenerator() override = default;

  /// @brief Interface method to generate a geometry id cache
  /// @return a geometry id cache wrapped in a std::any object
  IGeometryIdGenerator::GeoIdCache generateCache() const final;

  /// @brief Method for assigning a geometry id to a detector volume
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param dVolume the detector volume to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& cache,
                        DetectorVolume& dVolume) const final;

  /// @brief Method for assigning a geometry id to a portal
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param portal the portal to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& cache,
                        Portal& portal) const final;

  /// @brief Method for assigning a geometry id to a surface
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param surface the surface to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& cache,
                        Surface& surface) const final;

 private:
  /// @brief Helper method to get the volume id from the cache
  ///
  /// @param cache the provided cache
  /// @param incrementLayer if true, the layer counter is incremented
  ///
  /// @return a valid geometry identifier
  GeometryIdentifier volumeId(Cache& cache, bool incrementLayer = true) const;

  /// Configuration object
  Config m_cfg;

  /// Private access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Logger> m_logger;
};

/// This is a chained tgeometry id generator that will be in seuqnce
/// @tparam generators_t the generators that will be called in sequence
///
/// @note the generators are expected to be of pointer type
template <typename... generators_t>
class ChainedGeometryIdGenerator : public IGeometryIdGenerator {
 public:
  struct Cache {
    /// The caches
    std::array<IGeometryIdGenerator::GeoIdCache, sizeof...(generators_t)>
        storage;
  };

  /// The stored generators
  std::tuple<generators_t...> generators;

  /// Constructor for chained generators_t in a tuple, this will unroll
  /// the tuple and call them in sequence
  ///
  /// @param gens the updators to be called in chain
  /// @param mlogger is the logging instance
  ChainedGeometryIdGenerator(const std::tuple<generators_t...>&& gens,
                             std::unique_ptr<const Logger> mlogger =
                                 getDefaultLogger("ChainedGeometryIdGenerator",
                                                  Logging::INFO))
      : generators(std::move(gens)), m_logger(std::move(mlogger)) {}

  /// @brief Interface method to generate a geometry id cache
  /// @return a geometry id cache wrapped in a std::any object
  IGeometryIdGenerator::GeoIdCache generateCache() const final {
    // Unfold the tuple and add the attachers
    Cache cache;
    std::size_t it = 0;
    std::apply(
        [&](auto&&... generator) {
          ((cache.storage[it++] = generator->generateCache()), ...);
        },
        generators);
    return cache;
  }

  /// @brief Method for assigning a geometry id to a detector volume
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param dVolume the detector volume to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& cache,
                        DetectorVolume& dVolume) const final {
    ACTS_VERBOSE("Assigning chained geometry id to volume.");
    assign(cache, dVolume);
  }

  /// @brief Method for assigning a geometry id to a portal
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param portal the portal to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& cache,
                        Portal& portal) const final {
    ACTS_VERBOSE("Assigning chained geometry id to portal.");
    assign(cache, portal);
  }

  /// @brief Method for assigning a geometry id to a surface
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param surface the surface to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& cache,
                        Surface& surface) const final {
    ACTS_VERBOSE("Assigning chained geometry id to surface.");
    assign(cache, surface);
  }

 private:
  /// @brief Helper to run through the chain of generators
  ///
  /// @tparam gometry_object_t the geometry object type
  ///
  /// @param cache object with the array of sub caches
  /// @param object the object to assign the geometry id to
  template <typename gometry_object_t>
  void assign(IGeometryIdGenerator::GeoIdCache& cache,
              gometry_object_t& object) const {
    std::size_t it = 0;
    auto& sCache = std::any_cast<Cache&>(cache);
    std::apply(
        [&](auto&&... generator) {
          (generator->assignGeometryId(sCache.storage[it++], object), ...);
        },
        generators);
  }

  /// Private access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Experimental
}  // namespace Acts
