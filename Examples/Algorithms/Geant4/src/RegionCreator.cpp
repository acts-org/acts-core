// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Geant4/RegionCreator.hpp"

#include "Acts/Utilities/Logger.hpp"

#include <G4LogicalVolume.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4ProductionCuts.hh>
#include <G4Region.hh>

namespace ActsExamples::Geant4 {

RegionCreator::RegionCreator(const Config& cfg) : m_cfg(cfg) {}

G4Region* RegionCreator::buildRegion(const Acts::Logger& logger) const {
  // create a new G4Region
  G4Region* region = new G4Region(m_cfg.name);

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
                   << m_cfg.name
                   << " G4PhysicsRegion may not behave as intended.");
    }
    nVolumes += nVolumesCurrent;
  }

  ACTS_INFO("Created region " << m_cfg.name);
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

  return region;
}

}  // namespace ActsExamples::Geant4
