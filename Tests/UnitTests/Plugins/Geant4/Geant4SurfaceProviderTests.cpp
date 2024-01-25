// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/Geant4/Geant4SurfaceProvider.hpp"

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Detector/LayerStructureBuilder.hpp"

#include <filesystem>
#include <memory>
#include <string>

#include "G4Box.hh"
#include "G4GDMLParser.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4RotationMatrix.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4Transform3D.hh"

class G4VPhysicalVolume;

int nLayers = 5;
int nChips = 5;
int nArms = 2;

double cellDimX = 0.4 * cm;
double cellDimY = 0.3 * cm;
double cellDimZ = 0.1 * cm;

double armOffset = 10 * cm;

BOOST_AUTO_TEST_SUITE(Geant4SurfaceProvider)

std::tuple<G4VPhysicalVolume*, std::vector<std::string>>
ConstructGeant4World() {
  // Get nist material manager
  G4NistManager* nist = G4NistManager::Instance();

  // Option to switch on/off checking of volumes overlaps
  //
  G4bool checkOverlaps = true;

  //
  // World
  //
  G4double worldSizeXY = 50 * cm;
  G4double worldSizeZ = 50 * cm;
  G4Material* worldMat = nist->FindOrBuildMaterial("G4_Galactic");

  auto solidWorld = new G4Box("World", 0.5 * worldSizeXY, 0.5 * worldSizeXY,
                              0.5 * worldSizeZ);

  auto logicWorld = new G4LogicalVolume(solidWorld, worldMat, "World");

  auto physWorld = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld,
                                     "World", nullptr, false, 0, checkOverlaps);

  std::vector<std::string> names;
  for (int nArm = 0; nArm < nArms; nArm++) {
    for (int nLayer = 0; nLayer < nLayers; nLayer++) {
      for (int nChip = 0; nChip < nChips; nChip++) {
        int sign = (nArm == 0) ? 1 : -1;
        double posX = sign * (armOffset + nChip * cm);
        double posY = 0;
        double posZ = nLayer * cm;
        G4ThreeVector pos = G4ThreeVector(posX, posY, posZ);
        G4ThreeVector dims = G4ThreeVector(cellDimX, cellDimY, cellDimZ);

        int cellId = nChips * nLayers * nArm + nChips * nLayer + nChip;
        std::string name = "cell" + std::to_string(cellId);

        names.push_back(name);

        G4Material* material = nist->FindOrBuildMaterial("G4_He");

        // Box cell
        auto solidCell = new G4Box(name, dims[0], dims[1], dims[2]);

        G4LogicalVolume* cellLogical =
            new G4LogicalVolume(solidCell, material, "cellSensitive");

        new G4PVPlacement(nullptr, pos, cellLogical, name, logicWorld, false, 0,
                          true);
      }
    }
  }

  return std::make_tuple(physWorld, names);
}

auto gctx = Acts::GeometryContext();

BOOST_AUTO_TEST_CASE(Geant4SurfaceProviderNames) {
    // Construct the world
    auto [physWorld, names] = ConstructGeant4World();

    // Write GDML file
    std::filesystem::path gdmlPath = "test.gdml";
    G4GDMLParser parser;
    parser.Write(gdmlPath.string(), physWorld);

    // Default template parameters are fine
    // when using names as identifiers 
    auto spFullCfg = 
        Acts::Experimental::Geant4SurfaceProvider<>::Config();
    spFullCfg.gdmlPath = gdmlPath.string();
    spFullCfg.surfaceNames = names;
    spFullCfg.exactMatch = true;

    auto spFull =
        std::make_shared<Acts::Experimental::Geant4SurfaceProvider<>>(
            spFullCfg);

    auto lbFullCfg = Acts::Experimental::LayerStructureBuilder::Config();
    lbFullCfg.surfacesProvider = spFull;

    auto lbFull =
        std::make_shared<Acts::Experimental::LayerStructureBuilder>(
            lbFullCfg);

    auto [sFull, vFull, suFull, vuFull] =
        lbFull->construct(gctx);

    BOOST_CHECK_EQUAL(sFull.size(), names.size());
    for (int nArm = 0; nArm < nArms; nArm++) {
        for (int nLayer = 0; nLayer < nLayers; nLayer++) {
            for (int nChip = 0; nChip < nChips; nChip++) {
                int sign = (nArm == 0) ? 1 : -1;
                double posX = sign * (armOffset + nChip * cm);
                double posY = 0;
                double posZ = nLayer * cm;
                Acts::Vector3 pos = Acts::Vector3(posX, posY, posZ);
        
                BOOST_CHECK_EQUAL(
                    sFull.at(nChips * nLayers * nArm + nChips * nLayer + nChip)
                        ->center(gctx),
                    pos);
            }
        }
    }

    // Now check that we can extract only 
    // a subset of the surfaces
    std::vector<std::string> leftArmNames(names.begin(), names.begin() + names.size()/2);

    auto spLeftArmCfg = 
    Acts::Experimental::Geant4SurfaceProvider<>::Config();
    spLeftArmCfg.gdmlPath = gdmlPath.string();
    spLeftArmCfg.surfaceNames = leftArmNames;
    spLeftArmCfg.exactMatch = true;

    auto spLeftArm =
        std::make_shared<Acts::Experimental::Geant4SurfaceProvider<>>(
            spLeftArmCfg);

    auto lbCfg = Acts::Experimental::LayerStructureBuilder::Config();
    lbCfg.surfacesProvider = spLeftArm;

    auto lbLeftArm =
        std::make_shared<Acts::Experimental::LayerStructureBuilder>(
            lbCfg);

    auto [sLeftArm, vLeftArm, suLeftArm, vuLeftArm] =
        lbLeftArm->construct(gctx);

    BOOST_CHECK_EQUAL(sLeftArm.size(), leftArmNames.size());
    for (int nLayer = 0; nLayer < nLayers; nLayer++) {
        for (int nChip = 0; nChip < nChips; nChip++) {
            double posX = armOffset + nChip * cm;
            double posY = 0;
            double posZ = nLayer * cm;
            Acts::Vector3 pos = Acts::Vector3(posX, posY, posZ);
    
            BOOST_CHECK_EQUAL(
                sLeftArm.at(nChips * nLayer + nChip)->center(gctx),
                pos);
        }
    }

    std::filesystem::remove(gdmlPath);
}

BOOST_AUTO_TEST_CASE(Geant4SurfaceProviderRanges) {
    // Construct the world
    auto [physWorld, names] = ConstructGeant4World();

    // Write GDML file
    std::filesystem::path gdmlPath = "test.gdml";
    G4GDMLParser parser;
    parser.Write(gdmlPath.string(), physWorld);

    // 1D selection -- select only the first row
    auto sp1DCfg = 
        Acts::Experimental::Geant4SurfaceProvider<1>::Config();
    sp1DCfg.gdmlPath = gdmlPath.string();
    sp1DCfg.surfaceNames = names;
    
    auto kdt1DOpt = 
        Acts::Experimental::Geant4SurfaceProvider<1>::kdtOptions();
    kdt1DOpt.range = Acts::RangeXD<1,Acts::ActsScalar>();
    kdt1DOpt.range[0].set(-5, 5);
    kdt1DOpt.binningValues = {Acts::BinningValue::binZ};

    auto sp1D =
        std::make_shared<Acts::Experimental::Geant4SurfaceProvider<1>>(
            sp1DCfg, kdt1DOpt);

    auto lb1DCfg = Acts::Experimental::LayerStructureBuilder::Config();
    lb1DCfg.surfacesProvider = sp1D;

    auto lb1D =
        std::make_shared<Acts::Experimental::LayerStructureBuilder>(
            lb1DCfg);

    auto [s1D, v1D, su1D, vu1D] =
        lb1D->construct(gctx);

    BOOST_CHECK_EQUAL(s1D.size(), nChips*nArms);
    for (int nArm = 0; nArm < nArms; nArm++) {
        for (int nChip = 0; nChip < nChips; nChip++) {
            int sign = (nArm == 0) ? 1 : -1;
            double posX = sign * (armOffset + nChip * cm);
            double posY = 0, posZ = 0;
            Acts::Vector3 pos = Acts::Vector3(posX, posY, posZ);
        
            BOOST_CHECK_EQUAL(
                s1D.at(nChips * nArm + nChip)->center(gctx),
                pos);
        }
    }

    // 2D selection -- select only the first row
    // of the left arm
    auto sp2DCfg = 
        Acts::Experimental::Geant4SurfaceProvider<2>::Config();
    sp2DCfg.gdmlPath = gdmlPath.string();
    sp2DCfg.surfaceNames = names;
    
    auto kdt2DOpt = 
        Acts::Experimental::Geant4SurfaceProvider<2>::kdtOptions();
    kdt2DOpt.range = Acts::RangeXD<2,Acts::ActsScalar>();
    kdt2DOpt.range[0].set(-5, 5);
    kdt2DOpt.range[1].set(armOffset-5, armOffset+100);
    kdt2DOpt.binningValues = {Acts::BinningValue::binZ};

    auto sp2D =
        std::make_shared<Acts::Experimental::Geant4SurfaceProvider<2>>(
            sp2DCfg, kdt2DOpt);

    auto lb2DCfg = Acts::Experimental::LayerStructureBuilder::Config();
    lb2DCfg.surfacesProvider = sp2D;

    auto lb2D =
        std::make_shared<Acts::Experimental::LayerStructureBuilder>(
            lb2DCfg);

    auto [s2D, v2D, su2D, vu2D] =
        lb2D->construct(gctx);

    BOOST_CHECK_EQUAL(s2D.size(), nChips);
    for (int nChip = 0; nChip < nChips; nChip++) {
        double posX = armOffset + nChip * cm;
        double posY = 0, posZ = 0;
        Acts::Vector3 pos = Acts::Vector3(posX, posY, posZ);
    
        BOOST_CHECK_EQUAL(
            s2D.at(nChip)->center(gctx),pos);
    }

    std::filesystem::remove(gdmlPath);
}


BOOST_AUTO_TEST_SUITE_END()
