// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "PrimaryGeneratorAction.hpp"

#include "Acts/Definitions/Units.hpp"

#include <stdexcept>

#include <G4Event.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleGun.hh>
#include <G4RandomDirection.hh>
#include <G4UnitsTable.hh>
#include <Randomize.hh>

namespace ActsExamples::Geant4::HepMC3 {

PrimaryGeneratorAction* PrimaryGeneratorAction::s_instance = nullptr;

PrimaryGeneratorAction::PrimaryGeneratorAction(G4int randomSeed1,
                                               G4int randomSeed2)
    : G4VUserPrimaryGeneratorAction(), m_particleGun(nullptr) {
  // Configure the run
  if (s_instance != nullptr) {
    throw std::logic_error("Attempted to duplicate a singleton");
  } else {
    s_instance = this;
  }

  // Configure the gun
  G4int nofParticles = 1;
  m_particleGun = std::make_unique<G4ParticleGun>(nofParticles);

  // Prepare the particle table
  m_particleTable = G4ParticleTable::GetParticleTable();

  // Set the random seeds
  CLHEP::HepRandom::getTheEngine()->setSeed(randomSeed1, randomSeed2);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() {
  s_instance = nullptr;
}

PrimaryGeneratorAction* PrimaryGeneratorAction::instance() {
  // Static access function via G4RunManager
  return s_instance;
}

void PrimaryGeneratorAction::prepareParticleGun(
    const ActsExamples::SimParticle& part) {
  constexpr double convertLength = CLHEP::mm / Acts::UnitConstants::mm;
  constexpr double convertEnergy = CLHEP::GeV / Acts::UnitConstants::GeV;

  // Particle type
  G4ParticleDefinition* particle = m_particleTable->FindParticle(part.pdg());
  m_particleGun->SetParticleDefinition(particle);
  // Particle properties
  const auto pos = part.position() * convertLength;
  const auto dir = part.direction();
  m_particleGun->SetParticlePosition({pos[0], pos[1], pos[2]});
  m_particleGun->SetParticleMomentum(part.absoluteMomentum() * convertEnergy);
  m_particleGun->SetParticleMomentumDirection({dir[0], dir[1], dir[2]});
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent) {
  // Produce the event
  m_particleGun->GeneratePrimaryVertex(anEvent);
}

}  // namespace ActsExamples::Geant4::HepMC3
