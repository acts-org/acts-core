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

#include <memory>

#include <G4VUserDetectorConstruction.hh>

class G4VPhysicalVolume;

namespace dd4hep {
class Detector;
}
namespace ActsExamples {

namespace DD4hep {
struct DD4hepDetector;
}

/// Construct the Geant4 detector from a DD4hep description.
class DDG4DetectorConstruction final : public G4VUserDetectorConstruction {
 public:
  DDG4DetectorConstruction(
      std::shared_ptr<DD4hep::DD4hepDetector> detector,
      std::vector<std::shared_ptr<RegionCreator>> regionCreators = {});
  ~DDG4DetectorConstruction() final;

  /// Convert the stored DD4hep detector to a Geant4 description.
  ///
  /// Transfers ownership of the created object as all volumes (including world)
  /// are deleted in ~G4PhysicalVolumeStore().
  ///
  /// @note for facilitating configuration within the ACTS framework the world
  /// volume is cached
  G4VPhysicalVolume* Construct() final;

 private:
  /// The Acts DD4hep detector instance
  std::shared_ptr<DD4hep::DD4hepDetector> m_detector;
  /// Region creators
  std::vector<std::shared_ptr<RegionCreator>> m_regionCreators;
  /// The world volume
  G4VPhysicalVolume* m_world = nullptr;

  /// The DD4hep detector instance
  dd4hep::Detector& dd4hepDetector() const;
};

class DDG4DetectorConstructionFactory final
    : public DetectorConstructionFactory {
 public:
  DDG4DetectorConstructionFactory(
      std::shared_ptr<DD4hep::DD4hepDetector> detector,
      std::vector<std::shared_ptr<RegionCreator>> regionCreators = {});
  ~DDG4DetectorConstructionFactory() final;

  std::unique_ptr<G4VUserDetectorConstruction> factorize() const override;

 private:
  /// The Acts DD4hep detector instance
  std::shared_ptr<DD4hep::DD4hepDetector> m_detector;
  /// Region creators
  std::vector<std::shared_ptr<RegionCreator>> m_regionCreators;
};

}  // namespace ActsExamples
