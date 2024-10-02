// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/GdmlDetectorConstruction.hpp"

#include <utility>

#include <G4GDMLParser.hh>

class G4VPhysicalVolume;

using namespace ActsExamples;

GdmlDetectorConstruction::GdmlDetectorConstruction(
    std::string path,
    std::vector<std::shared_ptr<RegionCreator>> regionCreators)
    : G4VUserDetectorConstruction(),
      m_path(std::move(path)),
      m_regionCreators(std::move(regionCreators)) {}

G4VPhysicalVolume* GdmlDetectorConstruction::Construct() {
  if (m_world == nullptr) {
    G4GDMLParser parser;
    // TODO how to handle errors
    parser.Read(m_path);
    m_world = parser.GetWorldVolume();

    // Create regions
    for (const auto& regionCreator : m_regionCreators) {
      regionCreator->Construct();
    }
  }
  return m_world;
}

GdmlDetectorConstructionFactory::GdmlDetectorConstructionFactory(
    std::string path,
    std::vector<std::shared_ptr<RegionCreator>> regionCreators)
    : m_path(std::move(path)), m_regionCreators(std::move(regionCreators)) {}

std::unique_ptr<G4VUserDetectorConstruction>
GdmlDetectorConstructionFactory::factorize() const {
  return std::make_unique<GdmlDetectorConstruction>(m_path, m_regionCreators);
}
