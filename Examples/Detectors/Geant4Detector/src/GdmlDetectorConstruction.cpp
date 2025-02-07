// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Geant4Detector/GdmlDetectorConstruction.hpp"

#include "ActsExamples/Geant4/Geant4ConstructionOptions.hpp"
#include "ActsExamples/Geant4/RegionCreator.hpp"

#include <utility>

#include <G4GDMLParser.hh>

namespace ActsExamples {

GdmlDetectorConstruction::GdmlDetectorConstruction(
    std::string path, const Geant4ConstructionOptions& options)
    : G4VUserDetectorConstruction(),
      m_path(std::move(path)),
      m_options(options) {}

G4VPhysicalVolume* GdmlDetectorConstruction::Construct() {
  if (m_world == nullptr) {
    G4GDMLParser parser;
    // TODO how to handle errors
    parser.Read(m_path);
    m_world = parser.GetWorldVolume();

    // Create regions
    for (const auto& regionCreator : m_options.regionCreators) {
      regionCreator->buildRegion();
    }
  }
  return m_world;
}

}  // namespace ActsExamples
