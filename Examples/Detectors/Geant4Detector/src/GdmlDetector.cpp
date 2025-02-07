// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Geant4Detector/GdmlDetector.hpp"

#include "ActsExamples/Geant4Detector/GdmlDetectorConstruction.hpp"

#include <G4GDMLParser.hh>

namespace ActsExamples {

GdmlDetector::GdmlDetector(const Config& cfg)
    : Detector(Acts::getDefaultLogger("GdmlDetector", cfg.logLevel)),
      m_cfg(cfg) {}

std::unique_ptr<G4VUserDetectorConstruction>
GdmlDetector::buildGeant4DetectorConstruction(
    const Geant4ConstructionOptions& options) const {
  return std::make_unique<GdmlDetectorConstruction>(m_cfg.path, options);
}

}  // namespace ActsExamples
