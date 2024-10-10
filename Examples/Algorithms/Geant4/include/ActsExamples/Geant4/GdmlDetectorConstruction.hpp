// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Geant4/DetectorConstructionFactory.hpp"
#include "ActsExamples/Geant4/RegionCreator.hpp"

#include <string>

#include <G4VUserDetectorConstruction.hh>

class G4VPhysicalVolume;

namespace ActsExamples {

/// Construct the Geant4 detector from a Gdml file.
class GdmlDetectorConstruction final : public G4VUserDetectorConstruction {
 public:
  /// @param path is the path to the Gdml file
  /// @param regionCreators are the region creators
  GdmlDetectorConstruction(
      std::string path,
      std::vector<std::shared_ptr<RegionCreator>> regionCreators = {});

  /// Read the file and parse it to construct the Geant4 description
  ///
  /// @note to simplify further setup withiin the ACTS framework
  /// the world is cached when first constructed
  G4VPhysicalVolume* Construct() override;

 private:
  /// Path to the Gdml file
  std::string m_path;
  /// Region creators
  std::vector<std::shared_ptr<RegionCreator>> m_regionCreators;
  /// Cached world volume
  G4VPhysicalVolume* m_world = nullptr;
};

class GdmlDetectorConstructionFactory final
    : public DetectorConstructionFactory {
 public:
  GdmlDetectorConstructionFactory(
      std::string path,
      std::vector<std::shared_ptr<RegionCreator>> regionCreators = {});

  std::unique_ptr<G4VUserDetectorConstruction> factorize() const override;

 private:
  /// Path to the Gdml file
  std::string m_path;
  /// Region creators
  std::vector<std::shared_ptr<RegionCreator>> m_regionCreators;
};

}  // namespace ActsExamples
