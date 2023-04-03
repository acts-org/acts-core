// This file is part of the Acts project.
//
// Copyright (C) 2022-2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "G4VPhysicalVolume.hh"

namespace ActsExamples {

///@class MockupsectorBuilder
///
/// This is a class that builds a chamber by reading a gdml file

class MockupSectorBuilder {
 public:
  /// Nested configuration struct
  struct Config {
    // The path of the gdml file that holds the mockup geometry
    std::string gdml_path = "";

    // The number of sectors we want to create
    int NumberOfSectors = 1;
  };

  /// Nested configuration struct for chamber
  struct ChamberConfig {
    // The name of the chamber
    std::string name;

    // The names of the sensitive surfaces
    std::vector<std::string> SensitiveNames;

    // The names of the passive surfaces
    std::vector<std::string> PassiveNames;
  };

  /// Constructor
  ///@param config The configuration struct
  MockupSectorBuilder(const Config& config);

  /// Destructor
  ~MockupSectorBuilder() = default;

  /// Set the configuration object
  /// param config The configurtion struct
  void setConfiguration(const Config& config);

  /// Set the g4World from the gdml file
  void setWorld();

  /// Build chamber
  /// @param gctx The current geometry context object
  /// @param chamber_config The configuration chamber struct
  std::shared_ptr<Acts::Experimental::DetectorVolume> buildChamber(
      const Acts::GeometryContext& gctx, const ChamberConfig& chamber_config);

  /// Build Sector
  ///@param det_volumes The vector that contains the detector volumes of the Sector
  ///@param gctx The current geometry context object
  std::shared_ptr<Acts::Experimental::DetectorVolume> BuildSector(
      std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>
          det_volumes,
      const Acts::GeometryContext& gctx);

 private:
  Config m_cfg;

  G4VPhysicalVolume* g4World = nullptr;

  int maxNumberOfSectors = 8;
};

}  // namespace ActsExamples
