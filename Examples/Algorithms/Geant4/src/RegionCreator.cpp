// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/RegionCreator.hpp"

#include <G4LogicalVolume.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4ProductionCuts.hh>
#include <G4Region.hh>

namespace ActsExamples {

RegionCreator::RegionCreator(const Config& cfg, std::string name,
                             Acts::Logging::Level level)
    : m_name(std::move(name)),
      m_cfg(cfg),
      m_logger(Acts::getDefaultLogger(m_name, level)) {}

void RegionCreator::Construct() {
  // create a new G4Region
  G4Region* region = new G4Region(m_name);

  // loop over volumes and find the ones in the list
  std::size_t nVolumes{0};
  G4LogicalVolumeStore* logStore = G4LogicalVolumeStore::GetInstance();
  for (const std::string& volumeName : m_cfg.volumes) {
    std::size_t nVolumesCurrent{0};
    for (auto* it : *logStore) {
      ACTS_DEBUG("Checking volume " << it->GetName() << " against "
                                    << volumeName);
      if (volumeName == static_cast<const std::string&>(it->GetName())) {
        nVolumesCurrent++;
        it->SetRegion(region);
        region->AddRootLogicalVolume(it);
        ACTS_DEBUG("Volume " << it->GetName() << " added to region");
      }
    }
    if (nVolumesCurrent == 0) {
      ACTS_WARNING("No volumes matching \""
                   << volumeName << "\" found in G4 LogicalVolumeStore. "
                   << m_name << " G4PhysicsRegion may not behave as intended.");
    }
    nVolumes += nVolumesCurrent;
  }

  ACTS_INFO("Created region " << m_name);
  ACTS_INFO("A total of " << nVolumes << " volumes were assigned");

  // create a G4ProductionCuts object and set appropriate values
  G4ProductionCuts* cuts = new G4ProductionCuts();
  cuts->SetProductionCut(m_cfg.gammaCut, "gamma");
  cuts->SetProductionCut(m_cfg.electronCut, "e-");
  cuts->SetProductionCut(m_cfg.positronCut, "e+");
  cuts->SetProductionCut(m_cfg.protonCut, "proton");

  ACTS_INFO("Setting production cuts to");
  ACTS_INFO("    gamma: " << m_cfg.gammaCut);
  ACTS_INFO("    e-: " << m_cfg.electronCut);
  ACTS_INFO("    e+: " << m_cfg.positronCut);
  ACTS_INFO("    proton: " << m_cfg.protonCut);

  // assign cuts to the region
  region->SetProductionCuts(cuts);
}

}  // namespace ActsExamples
