// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

class G4VPhysicalVolume;

namespace Acts::Experimental {
class DetectorVolume;
}  // namespace Acts::Experimental

namespace ActsExamples {

///@class MockupsectorBuilder
///
/// This is a class that builds a chamber by reading a gdml file

class MockupSectorBuilder {
 public:
  /// Nested configuration struct
  struct Config {
    // The path of the gdml file that holds the mockup geometry
    std::string gdmlPath = "";

    // The number of sectors we want to create
    int NumberOfSectors = 1;

    float toleranceOverlap = 10.;
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

  /// Build chamber
  /// @param gctx The current geometry context object
  /// @param chamber_config The configuration chamber struct
  std::shared_ptr<Acts::Experimental::DetectorVolume> buildChamber(
      const ChamberConfig& chamberConfig);

  /// Build Sector
  /// @param det_volumes The vector that contains the detector volumes of the Sector
  /// @param gctx The current geometry context object
  std::shared_ptr<Acts::Experimental::DetectorVolume> buildSector(
      std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>
          detVolumes);

  /// Draw the sector in an obj file
  /// @param nameObjFile The name of the obj file where the sector will be saved
  void drawSector(const std::shared_ptr<Acts::Experimental::DetectorVolume>&
                      detectorVolumeSector,
                  const std::string& nameObjFile);

 private:
  Config mCfg;

  G4VPhysicalVolume* g4World = nullptr;

  int maxNumberOfSectors = 8;
};

}  // namespace ActsExamples
