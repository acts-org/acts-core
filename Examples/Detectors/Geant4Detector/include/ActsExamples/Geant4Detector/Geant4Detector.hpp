// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/ProtoDetector.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorSurfaceFactory.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>
#include <tuple>
#include <vector>

class G4VPhysicalVolume;

namespace Acts {
class TrackingGeometry;
class Geant4DetectorElement;
class Surface;

namespace Experimental {
class Detector;
}
}  // namespace Acts

namespace ActsExamples {
class IContextDecorator;

namespace Geant4 {

struct Geant4Detector {
  using DetectorElements =
      std::vector<std::shared_ptr<Acts::Geant4DetectorElement>>;
  using DetectorPtr = std::shared_ptr<Acts::Experimental::Detector>;
  using Surfaces = std::vector<std::shared_ptr<Acts::Surface>>;

  using ContextDecorators =
      std::vector<std::shared_ptr<ActsExamples::IContextDecorator>>;
  using TrackingGeometryPtr = std::shared_ptr<const Acts::TrackingGeometry>;

  /// Nested configuration struct
  struct Config {
    /// The detector/geometry name
    std::string name = "";
    /// The Geant4 world volume
    const G4VPhysicalVolume* g4World = nullptr;
    /// The Converter options: detector surfaces
    Acts::Geant4DetectorSurfaceFactory::Options g4SurfaceOptions;
    /// The corresponding ProtoDetector
    Acts::ProtoDetector protoDetector;
    /// Optional geometry identifier hook to be used during closure
    std::shared_ptr<const Acts::GeometryIdentifierHook> geometryIdentifierHook =
        std::make_shared<Acts::GeometryIdentifierHook>();
    /// Logging level of the child tools
    Acts::Logging::Level logLevel = Acts::Logging::INFO;
  };

  /// @brief Construct an Acts::Detector from a Geant4 world volume
  /// @param cfg the configuration of the Geant4 detector
  /// @param logger a logger instance
  /// @return a tuple of an Acts::Detector object, a ContextDecorator & the created detector elements
  std::tuple<DetectorPtr, ContextDecorators, DetectorElements>
  constructDetector(const Config& cfg, const Acts::Logger& logger);

  /// @brief Construct a TrackingGeometry from a Geant4 world volume using the KDTreeTrackingGeometryBuilder builder
  ///
  /// @param cfg the configuration of the Geant4 detector
  /// @param kdtCfg the configuration of the KDTreeTrackingGeometryBuilder
  /// @param logger a logger instance
  ///
  /// @return a tuple of an Acts::TrackingGeometry object,  a ContextDecorator & the created detector elements
  std::tuple<TrackingGeometryPtr, ContextDecorators, DetectorElements>
  constructTrackingGeometry(const Config& cfg, const Acts::Logger& logger);

 private:
  /// @brief Convert Geant4VPhysicalVolume objects into Acts components
  ///
  /// @param cfg the configuration of the Geant4 detector
  /// @param logger a logger instance
  ///
  /// @return a tuple of surfaces and detector elements
  std::tuple<Surfaces, DetectorElements> convertGeant4Volumes(
      const Config& cfg, const Acts::Logger& logger) const;
};

}  // namespace Geant4
}  // namespace ActsExamples
