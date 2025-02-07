// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Geant4/MaterialPhysicsList.hpp"

#include <utility>

#include <G4ParticleTypes.hh>
#include <G4ProcessManager.hh>
#include <G4ProcessVector.hh>
#include <G4UnitsTable.hh>

namespace ActsExamples::Geant4 {

MaterialPhysicsList::MaterialPhysicsList(
    std::unique_ptr<const Acts::Logger> logger)
    : G4VUserPhysicsList(), m_logger(std::move(logger)) {
  defaultCutValue = 1.0 * CLHEP::cm;
}

void MaterialPhysicsList::ConstructParticle() {
  ACTS_DEBUG("Construct Geantinos and Charged Geantinos.");
  G4Geantino::GeantinoDefinition();
  G4ChargedGeantino::ChargedGeantinoDefinition();
}

void MaterialPhysicsList::ConstructProcess() {
  ACTS_DEBUG("Adding Transport as single supperted Process.");
  AddTransportation();
}

void MaterialPhysicsList::SetCuts() {
  SetCutsWithDefault();

  if (verboseLevel > 0) {
    DumpCutValuesTable();
  }
}

}  // namespace ActsExamples::Geant4
