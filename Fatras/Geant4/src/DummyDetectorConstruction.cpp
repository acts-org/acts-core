// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsFatras/Geant4/DummyDetectorConstruction.hpp"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4PVPlacement.hh"
#include "G4RunManager.hh"
#include "G4ThreeVector.hh"
#include "QGSP_BERT.hh"

class G4VUserPhysicsList;

G4RunManager* ActsFatras::ensureGeant4RunManager() {
  // Test if there's already a G4RunManager
  if (G4RunManager::GetRunManager() == nullptr) {
    G4RunManager* runManager = new G4RunManager;

    // Initialise physics
    G4VUserPhysicsList* thePL = new QGSP_BERT;
    runManager->SetUserInitialization(thePL);

    // Build a dummy detector
    runManager->SetUserInitialization(new DummyDetectorConstruction());

    // Initialise the G4RunManager itself
    runManager->Initialize();
    return runManager;
  } else {
    // Return the existing G4RunManager
    return G4RunManager::GetRunManager();
  }
}

ActsFatras::DummyDetectorConstruction::~DummyDetectorConstruction() {
  delete (m_worldLog);
  delete (m_worldPhys);
}

void ActsFatras::DummyDetectorConstruction::dummyDetector() {
  G4ThreeVector materialPosition(0., 0., 0.);

  // Create the world setup
  G4Box* worldBox = new G4Box("WorldBox", 25000., 25000., 25000.);

  // G4 material: vacuum setup
  G4Material* g4vacuum = G4Material::GetMaterial("Vacuum", false);
  if (g4vacuum == nullptr) {
    g4vacuum =
        new G4Material("FatrasDummyVacuum", 1., 1.01 * CLHEP::g / CLHEP::mole,
                       CLHEP::universe_mean_density, kStateGas,
                       0.1 * CLHEP::kelvin, 1.e-19 * CLHEP::pascal);
  }

  // Build the logical and physical volume
  m_worldLog = m_worldLog != nullptr
                   ? new (m_worldLog)
                         G4LogicalVolume(worldBox, g4vacuum, "WorldLogical",
                                         nullptr, nullptr, nullptr)
                   : new G4LogicalVolume(worldBox, g4vacuum, "WorldLogical",
                                         nullptr, nullptr, nullptr);
  m_worldPhys =
      m_worldPhys != nullptr
          ? new (m_worldPhys)
                G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0),
                              "WorldPhysical", m_worldLog, nullptr, false, 0)
          : new G4PVPlacement(nullptr, materialPosition, "WorldPhysical",
                              m_worldLog, nullptr, false, 0);
}

G4VPhysicalVolume* ActsFatras::DummyDetectorConstruction::Construct() {
  // Construct the detector and return it
  dummyDetector();
  return m_worldPhys;
}
