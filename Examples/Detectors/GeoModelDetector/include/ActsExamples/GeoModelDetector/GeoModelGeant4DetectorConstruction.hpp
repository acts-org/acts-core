// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/GeoModel/GeoModelTree.hpp"
#include "ActsExamples/Geant4/Geant4ConstructionOptions.hpp"

#include <G4VUserDetectorConstruction.hh>

class G4VPhysicalVolume;

namespace ActsExamples {

/// Construct the Geant4 detector from a GeoModel world volume
class GeoModelGeant4DetectorConstruction final
    : public G4VUserDetectorConstruction {
 public:
  /// @param geoModelTree is the GeoModel tree containing the world volume
  /// @param regionCreators are the region creators
  GeoModelGeant4DetectorConstruction(const Acts::GeoModelTree& geoModelTree,
                                     const Geant4ConstructionOptions& options);

  /// Read the file and parse it to construct the Geant4 description
  ///
  /// @note to simplify further setup withiin the ACTS framework
  /// the world is cached when first constructed
  G4VPhysicalVolume* Construct() override;

 private:
  /// The GeoModel tree
  Acts::GeoModelTree m_geoModelTree;
  /// Construction options
  Geant4ConstructionOptions m_options;
  /// The world volume
  G4VPhysicalVolume* m_g4World = nullptr;
};

}  // namespace ActsExamples
