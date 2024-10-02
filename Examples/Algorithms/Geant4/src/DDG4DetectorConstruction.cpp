// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/DDG4/DDG4DetectorConstruction.hpp"

#include "ActsExamples/DD4hepDetector/DD4hepDetector.hpp"

#include <memory>
#include <vector>

#include <DD4hep/DetElement.h>
#include <DD4hep/Detector.h>
#include <DDG4/Geant4Converter.h>
#include <DDG4/Geant4GeometryInfo.h>
#include <DDG4/Geant4Mapping.h>
#include <Parsers/Printout.h>

class G4VPhysicalVolume;

ActsExamples::DDG4DetectorConstruction::DDG4DetectorConstruction(
    std::shared_ptr<DD4hep::DD4hepDetector> detector,
    std::vector<std::shared_ptr<RegionCreator>> regionCreators)
    : G4VUserDetectorConstruction(),
      m_detector(std::move(detector)),
      m_regionCreators(std::move(regionCreators)) {}

ActsExamples::DDG4DetectorConstruction::~DDG4DetectorConstruction() = default;

dd4hep::Detector& ActsExamples::DDG4DetectorConstruction::dd4hepDetector()
    const {
  return m_detector->geometryService->detector();
}

// See DD4hep::Simulation::Geant4DetectorConstruction::Construct()
G4VPhysicalVolume* ActsExamples::DDG4DetectorConstruction::Construct() {
  if (m_world == nullptr) {
    dd4hep::sim::Geant4Mapping& g4map = dd4hep::sim::Geant4Mapping::instance();
    auto conv = dd4hep::sim::Geant4Converter(dd4hepDetector(),
                                             dd4hep::PrintLevel::VERBOSE);
    dd4hep::sim::Geant4GeometryInfo* geoInfo =
        conv.create(dd4hepDetector().world()).detach();
    g4map.attach(geoInfo);
    // All volumes are deleted in ~G4PhysicalVolumeStore()
    m_world = geoInfo->world();
    // Create Geant4 volume manager
    g4map.volumeManager();

    // Create regions
    for (const auto& regionCreator : m_regionCreators) {
      regionCreator->Construct();
    }
  }
  return m_world;
}

ActsExamples::DDG4DetectorConstructionFactory::DDG4DetectorConstructionFactory(
    std::shared_ptr<DD4hep::DD4hepDetector> detector,
    std::vector<std::shared_ptr<RegionCreator>> regionCreators)
    : m_detector(std::move(detector)),
      m_regionCreators(std::move(regionCreators)) {}

ActsExamples::DDG4DetectorConstructionFactory::
    ~DDG4DetectorConstructionFactory() = default;

std::unique_ptr<G4VUserDetectorConstruction>
ActsExamples::DDG4DetectorConstructionFactory::factorize() const {
  return std::make_unique<DDG4DetectorConstruction>(m_detector,
                                                    m_regionCreators);
}
