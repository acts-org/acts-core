// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/GeoModel/GeoModelTree.hpp"
#include "ActsExamples/Geant4/DetectorConstructionFactory.hpp"
#include "ActsExamples/Geant4/RegionCreator.hpp"

#include <string>

#include <G4VUserDetectorConstruction.hh>

class G4VPhysicalVolume;

namespace ActsExamples {

/// Construct the Geant4 detector from a GeoModel world volume
class GeoModelDetectorConstruction final : public G4VUserDetectorConstruction {
 public:
  /// @param geoModelTree is the GeoModel tree containing the world volume
  /// @param regionCreators are the region creators
  GeoModelDetectorConstruction(
      const Acts::GeoModelTree& geoModelTree,
      std::vector<std::shared_ptr<RegionCreator>> regionCreators = {});

  /// Read the file and parse it to construct the Geant4 description
  ///
  /// @note to simplify further setup withiin the ACTS framework
  /// the world is cached when first constructed
  G4VPhysicalVolume* Construct() override;

 private:
  /// The GeoModel tree
  Acts::GeoModelTree m_geoModelTree;
  /// Region creators
  std::vector<std::shared_ptr<RegionCreator>> m_regionCreators;
  /// The world volume
  G4VPhysicalVolume* m_g4World = nullptr;
};

class GeoModelDetectorConstructionFactory final
    : public DetectorConstructionFactory {
 public:
  GeoModelDetectorConstructionFactory(
      const Acts::GeoModelTree& geoModelTree,
      std::vector<std::shared_ptr<RegionCreator>> regionCreators = {});

  std::unique_ptr<G4VUserDetectorConstruction> factorize() const override;

 private:
  /// The GeoModel tree
  Acts::GeoModelTree m_geoModelTree;
  /// Region creators
  std::vector<std::shared_ptr<RegionCreator>> m_regionCreators;
};

}  // namespace ActsExamples
