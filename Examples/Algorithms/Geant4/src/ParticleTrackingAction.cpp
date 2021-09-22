// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/ParticleTrackingAction.hpp"

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Geant4/EventStoreRegistry.hpp"

#include <G4ParticleDefinition.hh>
#include <G4RunManager.hh>
#include <G4Track.hh>
#include <G4UnitsTable.hh>
#include <globals.hh>

ActsExamples::ParticleTrackingAction::ParticleTrackingAction(
    const Config& cfg, std::unique_ptr<const Acts::Logger> logger)
    : G4UserTrackingAction(), m_cfg(cfg), m_logger(std::move(logger)) {}

void ActsExamples::ParticleTrackingAction::PreUserTrackingAction(
    const G4Track* aTrack) {
  auto& eventData = EventStoreRegistry::eventData();
  eventData.particlesInitial.push_back(convert(*aTrack));
}

void ActsExamples::ParticleTrackingAction::PostUserTrackingAction(
    const G4Track* aTrack) {
  auto& eventData = EventStoreRegistry::eventData();
  eventData.particlesFinal.push_back(convert(*aTrack));
}

ActsExamples::SimParticle ActsExamples::ParticleTrackingAction::convert(
    const G4Track& aTrack) const {
  // Unit conversions G4->::ACTS
  constexpr double convertTime = Acts::UnitConstants::s / CLHEP::s;
  constexpr double convertLength = Acts::UnitConstants::mm / CLHEP::mm;
  constexpr double convertEnergy = Acts::UnitConstants::GeV / CLHEP::GeV;

  // Get all the information from the Track
  const G4ParticleDefinition* particleDef = aTrack.GetParticleDefinition();
  G4double mass = particleDef->GetPDGMass();
  G4double charge = particleDef->GetPDGCharge();
  G4int pdg = particleDef->GetPDGEncoding();
  G4int id = aTrack.GetTrackID();
  G4ThreeVector pPosition = convertLength * aTrack.GetPosition();
  G4double pTime = convertTime * aTrack.GetGlobalTime();
  G4ThreeVector pDirection = aTrack.GetMomentumDirection();
  G4double p = convertEnergy * aTrack.GetKineticEnergy();
  // Now create the Particle
  ActsExamples::SimParticle aParticle(SimBarcode(id), Acts::PdgParticle(pdg),
                                      charge, mass);
  aParticle.setPosition4(pPosition[0], pPosition[1], pPosition[2], pTime);
  aParticle.setDirection(pDirection[0], pDirection[1], pDirection[2]);
  aParticle.setAbsoluteMomentum(p);
  return aParticle;
}
