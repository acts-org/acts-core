// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>
#include <vector>

class G4VPhysicalVolume;

namespace Acts {
class TrackingGeometry;
}

namespace ActsExamples {

/// This Mapper takes a (non-const) Geant4 geometry and maps
/// it such that name will be containing the mapping prefix
/// and the Acts::GeometryIdentifier of the surface.
///
/// This allows to directly associate Geant4 hits to the sensitive
/// elements of the Acts::TrackingGeoemtry w/o map lookup.
class SensitiveSurfaceMapper {
 public:
  static constexpr const char* mappingPrefix = "ActsGeoID#";

  /// Configuration struct for the surface mapper
  struct Config {
    std::vector<std::string> materialMappings = {"Silicon"};
    std::vector<std::string> volumeMappings = {};

    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry = nullptr;
  };

  /// Constructor with:
  ///
  /// @param cfg the configuration struct
  /// @param logger the logging instance
  SensitiveSurfaceMapper(const Config& cfg,
                         std::unique_ptr<const Acts::Logger> logger =
                             Acts::getDefaultLogger("SensitiveSurfaceMapper",
                                                    Acts::Logging::INFO));
  ~SensitiveSurfaceMapper() = default;

  /// Recursive mapping function that walks through the Geant4
  /// hierarchy and applies name remapping to the Physical volumes
  /// of the Geant4 geometry.
  ///
  /// @param g4PhysicalVolume the current physical volume in process
  /// @param motherPosition the absolute position of the mother
  /// @param sCounter  a counter of how many volumes have been remapped
  void remapSensitiveNames(G4VPhysicalVolume* g4PhysicalVolume,
                           const Acts::Vector3 motherPosition,
                           int& sCounter) const;

 protected:
  /// Configuration object
  Config m_cfg;

 private:
  /// Private access method to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// The looging instance
  std::unique_ptr<const Acts::Logger> m_logger;
};

}  // namespace ActsExamples
