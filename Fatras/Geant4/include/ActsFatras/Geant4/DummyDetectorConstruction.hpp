// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "G4VUserDetectorConstruction.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;
class G4RunManager;

namespace ActsFatras {

/// Either return the global run manager or create a new dummy one.
///
/// @return Pointer to the run manager
G4RunManager* ensureGeant4RunManager();

/// Construct a dummy Geant4 detector.
class DummyDetectorConstruction : public G4VUserDetectorConstruction {
 public:
  /// Destructor
  ~DummyDetectorConstruction() override;

  /// Interface method for Geant4.
  G4VPhysicalVolume* Construct() override;

 private:
  /// This method builds a dummy Geant4 detector.
  void dummyDetector();

  // Logical volume
  G4LogicalVolume* m_worldLog = nullptr;

  // Physical volume
  G4VPhysicalVolume* m_worldPhys = nullptr;
};
}  // namespace ActsFatras
