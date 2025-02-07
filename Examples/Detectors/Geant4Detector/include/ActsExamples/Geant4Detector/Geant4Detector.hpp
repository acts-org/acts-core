// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Detector/ProtoDetector.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorSurfaceFactory.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/DetectorCommons/Detector.hpp"

#include <memory>
#include <string>
#include <tuple>
#include <vector>

class G4VPhysicalVolume;

namespace ActsExamples {

struct Geant4Detector : public Detector {
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

  /// @brief Convert Geant4VPhysicalVolume objects into Acts components
  ///
  /// @param cfg the configuration of the Geant4 detector
  /// @param logger a logger instance
  ///
  /// @return a tuple of surfaces and detector elements
  static std::tuple<std::vector<std::shared_ptr<Acts::Surface>>,
                    std::vector<std::shared_ptr<Acts::Geant4DetectorElement>>>
  buildGeant4Volumes(const Config& cfg, const Acts::Logger& logger);

  explicit Geant4Detector(const Config& cfg);

 private:
  Config m_cfg;
};

}  // namespace ActsExamples
