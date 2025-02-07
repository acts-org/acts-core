// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/EventData/SimParticle.hpp"

#include <memory>

#include <G4ParticleTable.hh>
#include <G4SystemOfUnits.hh>
#include <G4ThreeVector.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <globals.hh>

class G4ParticleGun;
class G4Event;

namespace ActsExamples::Geant4::HepMC3 {

/// The PrimaryGeneratorAction is the implementation of the Geant4
/// class G4VUserPrimaryGeneratorAction. It generates a random direction
/// and shoots a particle.
class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
 public:
  /// Constructor
  PrimaryGeneratorAction(G4int randomSeed1 = 12345, G4int randomSeed2 = 23456);
  /// Destructor
  ~PrimaryGeneratorAction() override;

  /// Static access method
  static PrimaryGeneratorAction* instance();

  /// Interface method to generate the primary
  void GeneratePrimaries(G4Event* event) override;

  /// Prepare the particle gun with initial parameters
  void prepareParticleGun(const ActsExamples::SimParticle& part);

 private:
  /// Instance of the PrimaryGeneratorAction
  static PrimaryGeneratorAction* s_instance;

  /// Pointer to the G4 particle gun
  std::unique_ptr<G4ParticleGun> m_particleGun;
  /// The Geant4 particle table
  G4ParticleTable* m_particleTable;
};

}  // namespace ActsExamples::Geant4::HepMC3
