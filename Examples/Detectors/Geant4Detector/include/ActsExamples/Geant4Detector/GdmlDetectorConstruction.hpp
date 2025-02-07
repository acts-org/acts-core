// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/Geant4/Geant4ConstructionOptions.hpp"

#include <string>

#include <G4VUserDetectorConstruction.hh>

class G4VPhysicalVolume;

namespace ActsExamples {

/// Construct the Geant4 detector from a Gdml file.
class GdmlDetectorConstruction final : public G4VUserDetectorConstruction {
 public:
  /// @param path is the path to the Gdml file
  /// @param regionCreators are the region creators
  GdmlDetectorConstruction(std::string path,
                           const Geant4ConstructionOptions& options);

  /// Read the file and parse it to construct the Geant4 description
  ///
  /// @note to simplify further setup withiin the ACTS framework
  /// the world is cached when first constructed
  G4VPhysicalVolume* Construct() override;

 private:
  /// Path to the Gdml file
  std::string m_path;
  /// Construction options
  Geant4ConstructionOptions m_options;
  /// Cached world volume
  G4VPhysicalVolume* m_world = nullptr;
};

}  // namespace ActsExamples
