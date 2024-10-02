// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/Geant4/Geant4PhysicalVolumeSelectors.hpp"

#include <string>

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"

class G4VPhysicalVolume;

BOOST_AUTO_TEST_SUITE(Geant4Plugin)

BOOST_AUTO_TEST_CASE(Geant4PhysicalVolumeSelectors_test) {
  G4Box* worldS = new G4Box("world", 100, 100, 100);

  G4LogicalVolume* worldLV = new G4LogicalVolume(worldS, nullptr, "World");

  G4VPhysicalVolume* worldPV = new G4PVPlacement(
      nullptr, G4ThreeVector(), worldLV, "World", nullptr, false, 0, true);

  G4Box* boxS = new G4Box("box", 10, 10, 10);
  G4LogicalVolume* boxLV = new G4LogicalVolume(boxS, nullptr, "World");
  G4VPhysicalVolume* boxPV = new G4PVPlacement(nullptr, G4ThreeVector(), boxLV,
                                               "Box", worldLV, false, 0, true);

  auto allSelector = Acts::Geant4PhysicalVolumeSelectors::AllSelector();
  BOOST_CHECK(allSelector.select(*worldPV));
  BOOST_CHECK(allSelector.select(*boxPV));

  auto nameSelector =
      Acts::Geant4PhysicalVolumeSelectors::NameSelector({"ox"}, false);
  BOOST_CHECK(!nameSelector.select(*worldPV));
  BOOST_CHECK(nameSelector.select(*boxPV));

  auto nameSelectorE =
      Acts::Geant4PhysicalVolumeSelectors::NameSelector({"ox"}, true);
  BOOST_CHECK(!nameSelectorE.select(*worldPV));
  BOOST_CHECK(!nameSelectorE.select(*boxPV));
}

BOOST_AUTO_TEST_SUITE_END()
