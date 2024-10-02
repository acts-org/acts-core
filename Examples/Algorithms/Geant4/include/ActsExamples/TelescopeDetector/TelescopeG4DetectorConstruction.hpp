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
#include "ActsExamples/TelescopeDetector/TelescopeDetector.hpp"

#include "G4VUserDetectorConstruction.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;

namespace ActsExamples::Telescope {

class TelescopeG4DetectorConstruction final
    : public G4VUserDetectorConstruction {
 public:
  TelescopeG4DetectorConstruction(
      const TelescopeDetector::Config& cfg,
      std::vector<std::shared_ptr<RegionCreator>> regionCreators = {});

  G4VPhysicalVolume* Construct() final;

 private:
  /// The configuration of the telescope detector
  TelescopeDetector::Config m_cfg;
  /// Region creators
  std::vector<std::shared_ptr<RegionCreator>> m_regionCreators;
  /// The world volume
  G4VPhysicalVolume* m_world{};
};

class TelescopeG4DetectorConstructionFactory final
    : public DetectorConstructionFactory {
 public:
  TelescopeG4DetectorConstructionFactory(
      const TelescopeDetector::Config& cfg,
      std::vector<std::shared_ptr<RegionCreator>> regionCreators = {});

  std::unique_ptr<G4VUserDetectorConstruction> factorize() const override;

 private:
  /// The configuration of the telescope detector
  TelescopeDetector::Config m_cfg;
  /// Region creators
  std::vector<std::shared_ptr<RegionCreator>> m_regionCreators;
};

}  // namespace ActsExamples::Telescope
