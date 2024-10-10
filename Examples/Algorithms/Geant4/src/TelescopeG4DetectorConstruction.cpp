// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TelescopeDetector/TelescopeG4DetectorConstruction.hpp"

#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"
#include "ActsExamples/TelescopeDetector/BuildTelescopeDetector.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"

ActsExamples::Telescope::TelescopeG4DetectorConstruction::
    TelescopeG4DetectorConstruction(
        const TelescopeDetector::Config& cfg,
        std::vector<std::shared_ptr<RegionCreator>> regionCreators)
    : m_cfg(cfg), m_regionCreators(std::move(regionCreators)) {
  throw_assert(cfg.surfaceType ==
                   static_cast<int>(Telescope::TelescopeSurfaceType::Plane),
               "only plan is supported right now");
}

G4VPhysicalVolume*
ActsExamples::Telescope::TelescopeG4DetectorConstruction::Construct() {
  if (m_world != nullptr) {
    return m_world;
  }

  G4double center =
      (m_cfg.positions.back() + m_cfg.positions.front()) * 0.5 * mm;
  G4double length = (m_cfg.positions.back() - m_cfg.positions.front()) * mm;

  // Get nist material manager
  G4NistManager* nist = G4NistManager::Instance();

  // World
  //
  G4double worldSize =
      std::max({std::abs(m_cfg.offsets[0]) + m_cfg.bounds[0] * 0.5,
                std::abs(m_cfg.offsets[1]) + m_cfg.bounds[1] * 0.5,
                m_cfg.positions.back() + m_cfg.thickness});

  // Envelope parameters
  //
  G4double envSizeX = m_cfg.bounds[0] * mm;
  G4double envSizeY = m_cfg.bounds[1] * mm;
  G4double envSizeZ = length + m_cfg.thickness * mm;

  // Option to switch on/off checking of volumes overlaps
  //
  G4bool checkOverlaps = true;

  // Materials
  G4Material* galactic = nist->FindOrBuildMaterial("G4_Galactic");
  G4Material* silicon =
      new G4Material("Silicon", 14, 28.0855 * g / mole, 2.329 * g / cm3);

  // Construct the rotation
  // This assumes the binValue is BinningValue::binX, BinningValue::binY or
  // BinningValue::binZ. No reset is necessary in case of BinningValue::binZ
  G4RotationMatrix* rotation = nullptr;
  if (static_cast<Acts::BinningValue>(m_cfg.binValue) ==
      Acts::BinningValue::binX) {
    rotation = new G4RotationMatrix({0, 0, 1}, {0, 1, 0}, {-1, 0, 0});
  } else if (static_cast<Acts::BinningValue>(m_cfg.binValue) ==
             Acts::BinningValue::binY) {
    rotation = new G4RotationMatrix({1, 0, 0}, {0, 0, 1}, {0, -1, 0});
  }

  // World
  //
  G4Box* solidWorld = new G4Box("World Solid", worldSize, worldSize, worldSize);

  G4LogicalVolume* logicWorld =
      new G4LogicalVolume(solidWorld, galactic, "World Logic");

  m_world = new G4PVPlacement(nullptr,          // no rotation
                              G4ThreeVector(),  // position
                              logicWorld,       // its logical volume
                              "World Phys",     // its name
                              nullptr,          // its mother volume
                              false,            // no boolean operation
                              0,                // copy number
                              checkOverlaps);   // overlaps checking

  // Envelope 1
  //
  G4Box* solidEnv =
      new G4Box("Envelope Solid",                                 // its name
                0.5 * envSizeX, 0.5 * envSizeY, 0.5 * envSizeZ);  // its size

  G4LogicalVolume* logicEnv1 =
      new G4LogicalVolume(solidEnv,              // its solid
                          galactic,              // its material
                          "Envelope #1 Logic");  // its name

  G4VPhysicalVolume* physEnv1 =
      new G4PVPlacement(rotation,            // rotation
                        G4ThreeVector(),     // at detector center
                        logicEnv1,           // its logical volume
                        "Envelope #1 Phys",  // its name
                        logicWorld,          // its mother volume
                        false,               // no boolean operation
                        0,                   // copy number
                        checkOverlaps);      // overlaps checking

  // Envelope 2
  //
  G4LogicalVolume* logicEnv2 =
      new G4LogicalVolume(solidEnv,              // its solid
                          galactic,              // its material
                          "Envelope #2 Logic");  // its name

  G4VPhysicalVolume* physEnv2 = new G4PVPlacement(
      nullptr,  // no rotation
      G4ThreeVector(m_cfg.offsets[0] * mm, m_cfg.offsets[1] * mm,
                    center),  // at detector center
      "Envelope #2 Phys",     // its name
      logicEnv2,              // its logical volume
      physEnv1,               // its mother volume
      false,                  // no boolean operation
      0,                      // copy number
      checkOverlaps);         // overlaps checking

  // Layer
  //

  G4Box* solidLayer = new G4Box("Layer Solid", 0.5 * m_cfg.bounds[0],
                                0.5 * m_cfg.bounds[1], 0.5 * m_cfg.thickness);

  G4LogicalVolume* logicLayer = new G4LogicalVolume(solidLayer,  // its solid
                                                    silicon,     // its material
                                                    "Layer Logic");  // its name

  for (std::size_t i = 0; i < m_cfg.positions.size(); ++i) {
    new G4PVPlacement(
        nullptr,                                                // no rotation
        G4ThreeVector(0, 0, m_cfg.positions[i] * mm - center),  // at position
        "Layer #" + std::to_string(i) + " Phys",                // its name
        logicLayer,      // its logical volume
        physEnv2,        // its mother volume
        false,           // no boolean operation
        0,               // copy number
        checkOverlaps);  // overlaps checking
  }

  // Create regions
  for (const auto& regionCreator : m_regionCreators) {
    regionCreator->Construct();
  }

  return m_world;
}

ActsExamples::Telescope::TelescopeG4DetectorConstructionFactory::
    TelescopeG4DetectorConstructionFactory(
        const TelescopeDetector::Config& cfg,
        std::vector<std::shared_ptr<RegionCreator>> regionCreators)
    : m_cfg(cfg), m_regionCreators(std::move(regionCreators)) {}

std::unique_ptr<G4VUserDetectorConstruction>
ActsExamples::Telescope::TelescopeG4DetectorConstructionFactory::factorize()
    const {
  return std::make_unique<TelescopeG4DetectorConstruction>(m_cfg,
                                                           m_regionCreators);
}
