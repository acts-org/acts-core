// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/IMaterialDecorator.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Plugins/Json/MaterialMapJsonConverter.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <fstream>
#include <map>
#include <mutex>

// Convenience shorthand

namespace Acts {

/// @brief Material decorator from Json format
///
/// This reads in material maps for surfaces and volumes
/// from a json file
class JsonMaterialDecorator : public IMaterialDecorator {
 public:
  using SurfaceMaterialMap =
      std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>;

  using VolumeMaterialMap =
      std::map<GeometryIdentifier, std::shared_ptr<const IVolumeMaterial>>;

  JsonMaterialDecorator(const MaterialMapJsonConverter::Config& rConfig,
                        const std::string& jFileName,
                        Acts::Logging::Level level,
                        bool clearSurfaceMaterial = true,
                        bool clearVolumeMaterial = true);

  /// Decorate a surface
  ///
  /// @param surface the non-cost surface that is decorated
  void decorate(Surface& surface) const final;

  /// Decorate a TrackingVolume
  ///
  /// @param volume the non-cost volume that is decorated
  void decorate(TrackingVolume& volume) const final;

 private:
  MaterialMapJsonConverter::Config m_readerConfig;
  SurfaceMaterialMap m_surfaceMaterialMap;
  VolumeMaterialMap m_volumeMaterialMap;

  bool m_clearSurfaceMaterial{true};
  bool m_clearVolumeMaterial{true};

  std::unique_ptr<const Logger> m_logger;

  const Logger& logger() const { return *m_logger; }
};
}  // namespace Acts
