// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/SimParticleTranslation.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Geant4/EventStore.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>

#include <G4ChargedGeantino.hh>
#include <G4Event.hh>
#include <G4Geantino.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>
#include <G4PrimaryParticle.hh>
#include <G4PrimaryVertex.hh>
#include <G4UnitsTable.hh>

namespace ActsExamples {
class WhiteBoard;
}  // namespace ActsExamples

ActsExamples::SimParticleTranslation::SimParticleTranslation(
    const Config& cfg, std::unique_ptr<const Acts::Logger> logger)
    : G4VUserPrimaryGeneratorAction(),
      m_cfg(cfg),
      m_logger(std::move(logger)) {}

ActsExamples::SimParticleTranslation::~SimParticleTranslation() = default;

void ActsExamples::SimParticleTranslation::GeneratePrimaries(G4Event* anEvent) {
  anEvent->SetEventID(m_eventNr++);
  unsigned int eventID = anEvent->GetEventID();

  ACTS_DEBUG("Primary Generator Action for Event: " << eventID);

  if (eventStore().store == nullptr) {
    ACTS_WARNING("No WhiteBoard instance could be found for this event!");
    return;
  }

  if (eventStore().inputParticles == nullptr) {
    ACTS_WARNING("No input particle handle found");
    return;
  }

  // Get the number of input particles
  const auto inputParticles =
      (*eventStore().inputParticles)(*eventStore().store);

  // Reserve hopefully enough hit space
  eventStore().hits.reserve(inputParticles.size() *
                            m_cfg.reserveHitsPerParticle);

  // Default particle kinematic
  G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  G4PrimaryVertex* pVertex = nullptr;

  // We are looping through the particles and flush per vertex
  std::optional<Acts::Vector4> lastVertex;

  constexpr double convertLength = CLHEP::mm / Acts::UnitConstants::mm;
  constexpr double convertTime = CLHEP::ns / Acts::UnitConstants::ns;
  constexpr double convertEnergy = CLHEP::GeV / Acts::UnitConstants::GeV;

  unsigned int pCounter = 0;
  unsigned int trackId = 1;
  // Loop over the input partilces and run
  for (const auto& part : inputParticles) {
    auto currentVertex = part.fourPosition();
    if (!lastVertex || !currentVertex.isApprox(*lastVertex)) {
      // Add the vertex to the event
      if (pVertex != nullptr) {
        anEvent->AddPrimaryVertex(pVertex);
        ACTS_DEBUG("Flushing " << pCounter
                               << " particles associated with vertex "
                               << lastVertex->transpose());
        pCounter = 0;
      }
      lastVertex = currentVertex;
      pVertex = new G4PrimaryVertex(
          currentVertex[0] * convertLength, currentVertex[1] * convertLength,
          currentVertex[2] * convertLength, currentVertex[3] * convertTime);
    }

    // Add a new primary to the vertex

    Acts::Vector4 mom4 = part.fourMomentum() * convertEnergy;

    // Particle properties, may be forced to specific value
    G4int particlePdgCode = m_cfg.forcedPdgCode.value_or(part.pdg());
    G4double particleCharge = m_cfg.forcedCharge.value_or(part.charge());
    G4double particleMass =
        m_cfg.forcedMass.value_or(part.mass() * convertEnergy);

    // Check if it is a Geantino / ChargedGeantino
    G4ParticleDefinition* particleDefinition =
        particleTable->FindParticle(particlePdgCode);
    if (particleDefinition == nullptr) {
      if (particlePdgCode == 0 && particleMass == 0 && particleCharge == 0) {
        particleDefinition = G4Geantino::Definition();
      }
      if (particlePdgCode == 0 && particleMass == 0 && particleCharge != 0) {
        if (particleCharge != 1) {
          ACTS_ERROR("invalid charged geantino charge " << particleCharge
                                                        << ". should be 1");
        }
        particleDefinition = G4ChargedGeantino::Definition();
      }
    }

    // Skip if translation failed
    if (particleDefinition == nullptr) {
      ACTS_DEBUG(
          "Could not translate particle with PDG code : " << particlePdgCode);
      continue;
    }

    ACTS_VERBOSE("Adding particle with name '"
                 << particleDefinition->GetParticleName()
                 << "' and properties:");
    ACTS_VERBOSE(" -> mass: " << particleMass);
    ACTS_VERBOSE(" -> charge: " << particleCharge);
    ACTS_VERBOSE(" -> momentum: " << mom4.transpose());

    // G4 will delete this
    G4PrimaryParticle* particle = new G4PrimaryParticle(particleDefinition);

    particle->SetMass(particleMass);
    particle->SetCharge(particleCharge);
    particle->Set4Momentum(mom4[0], mom4[1], mom4[2], mom4[3]);
    particle->SetTrackID(trackId++);

    // Add the primary to the vertex
    pVertex->SetPrimary(particle);

    eventStore().particlesInitial.insert(part);
    eventStore().trackIdMapping[particle->GetTrackID()] = part.particleId();

    ++pCounter;
  }
  // Final vertex to be added
  if (pVertex != nullptr) {
    anEvent->AddPrimaryVertex(pVertex);
    ACTS_DEBUG("Flushing " << pCounter << " particles associated with vertex "
                           << lastVertex->transpose());
  }
}
