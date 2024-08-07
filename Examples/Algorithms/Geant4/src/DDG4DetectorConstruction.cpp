// This file is part of the Acts project.
//
// Copyright (C) 2017-2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/DDG4/DDG4DetectorConstruction.hpp"

#include "ActsExamples/DD4hepDetector/DD4hepGeometryService.hpp"

#include <memory>
#include <vector>

#include <DD4hep/DetElement.h>
#include <DD4hep/Detector.h>
#include <DDG4/Geant4Converter.h>
#include <DDG4/Geant4GeometryInfo.h>
#include <DDG4/Geant4Mapping.h>

class G4VPhysicalVolume;

namespace ActsExamples {

DDG4DetectorConstruction::DDG4DetectorConstruction(
    std::shared_ptr<DD4hep::DD4hepGeometryService> geometryService,
    std::vector<std::shared_ptr<RegionCreator>> regionCreators)
    : G4VUserDetectorConstruction(),
      m_geometryService(std::move(geometryService)),
      m_regionCreators(std::move(regionCreators)) {}

// See DD4hep::Simulation::Geant4DetectorConstruction::Construct()
G4VPhysicalVolume* DDG4DetectorConstruction::Construct() {
  if (m_world == nullptr) {
    dd4hep::sim::Geant4Mapping& g4map = dd4hep::sim::Geant4Mapping::instance();
    auto conv = dd4hep::sim::Geant4Converter(m_geometryService->detector(),
                                             dd4hep::PrintLevel::VERBOSE);
    dd4hep::sim::Geant4GeometryInfo* geo_info =
        conv.create(m_geometryService->detector().world()).detach();
    g4map.attach(geo_info);
    // All volumes are deleted in ~G4PhysicalVolumeStore()
    m_world = geo_info->world();
    // Create Geant4 volume manager
    g4map.volumeManager();

    // Create regions
    for (const auto& regionCreator : m_regionCreators) {
      regionCreator->Construct();
    }
  }
  return m_world;
}

DDG4DetectorConstructionFactory::DDG4DetectorConstructionFactory(
    std::shared_ptr<DD4hep::DD4hepGeometryService> geometryService,
    std::vector<std::shared_ptr<RegionCreator>> regionCreators)
    : m_geometryService(std::move(geometryService)),
      m_regionCreators(std::move(regionCreators)) {}

DDG4DetectorConstructionFactory::~DDG4DetectorConstructionFactory() = default;

std::unique_ptr<G4VUserDetectorConstruction>
DDG4DetectorConstructionFactory::factorize() const {
  return std::make_unique<DDG4DetectorConstruction>(m_geometryService,
                                                    m_regionCreators);
}

}  // namespace ActsExamples
