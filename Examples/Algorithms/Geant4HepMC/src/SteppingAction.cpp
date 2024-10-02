// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "SteppingAction.hpp"

#include "Acts/Utilities/Helpers.hpp"

#include <stdexcept>

#include <G4RunManager.hh>
#include <G4Step.hh>
#include <G4VProcess.hh>
#include <HepMC3/Attribute.h>
#include <HepMC3/Units.h>

#include "EventAction.hpp"

namespace ActsExamples::Geant4::HepMC3 {

SteppingAction* SteppingAction::s_instance = nullptr;

SteppingAction* SteppingAction::instance() {
  // Static access function via G4RunManager
  return s_instance;
}

SteppingAction::SteppingAction(std::vector<std::string> eventRejectionProcess)
    : G4UserSteppingAction(),
      m_eventRejectionProcess(std::move(eventRejectionProcess)) {
  if (s_instance != nullptr) {
    throw std::logic_error("Attempted to duplicate a singleton");
  } else {
    s_instance = this;
  }
}

SteppingAction::~SteppingAction() {
  s_instance = nullptr;
}

void SteppingAction::UserSteppingAction(const G4Step* step) {
  // Test if the event should be aborted
  if (Acts::rangeContainsValue(m_eventRejectionProcess,
                               step->GetPostStepPoint()
                                   ->GetProcessDefinedStep()
                                   ->GetProcessName())) {
    m_eventAborted = true;
    G4RunManager::GetRunManager()->AbortEvent();
    return;
  }

  /// Store the step such that a vertex knows the position and upcoming process
  /// for a particle. The particle properties are stored as ingoing before and
  /// as outgoing after the step with the process was performed. Therefore the
  /// vertex defines the starting point of a process while the next vertex
  /// describes the state after the process, including all particles produced
  /// along the step. In total the entire event is represented by vertices which
  /// describe each step in Geant4.

  ::HepMC3::GenEvent& event = EventAction::instance()->event();

  // Unit conversions G4->::HepMC3
  constexpr double convertLength = 1. / CLHEP::mm;
  constexpr double convertEnergy = 1. / CLHEP::GeV;
  constexpr double convertTime = 1. / CLHEP::s;

  // The particle after the step
  auto* track = step->GetTrack();
  const std::string trackId = std::to_string(track->GetTrackID());
  auto postStepMomentum = track->GetMomentum() * convertEnergy;
  auto postStepEnergy = track->GetTotalEnergy() * convertEnergy;
  ::HepMC3::FourVector mom4{postStepMomentum[0], postStepMomentum[1],
                            postStepMomentum[2], postStepEnergy};
  auto postParticle = std::make_shared<::HepMC3::GenParticle>(
      mom4, track->GetDynamicParticle()->GetPDGcode());

  // The process that led to the current state
  auto process = std::make_shared<::HepMC3::StringAttribute>(
      step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName());

  // Assign particle to a production vertex
  if (!m_previousVertex) {
    // Get the production position of the particle
    auto* preStep = step->GetPreStepPoint();
    auto prePosition = preStep->GetPosition() * convertLength;
    auto preTime = preStep->GetGlobalTime() * convertTime;
    ::HepMC3::FourVector prePos{prePosition[0], prePosition[1], prePosition[2],
                                preTime};

    // Handle the first step: No vertices exist
    if (event.vertices().empty()) {
      auto vertex = std::make_shared<::HepMC3::GenVertex>(prePos);
      vertex->add_particle_out(postParticle);
      event.add_vertex(vertex);
      vertex->set_status(1);
      vertex->add_attribute("NextProcessOf" + trackId, process);
    } else {
      // Search for an existing vertex
      for (const auto& vertex : event.vertices()) {
        if (vertex->position() == prePos) {
          // Add particle to existing vertex
          vertex->add_particle_out(postParticle);
          vertex->add_attribute("NextProcessOf-" + trackId, process);
          auto preStepMomentum =
              step->GetPreStepPoint()->GetMomentum() * convertEnergy;
          auto preStepEnergy =
              step->GetPreStepPoint()->GetTotalEnergy() * convertEnergy;
          auto preMom4 = std::make_shared<::HepMC3::VectorDoubleAttribute>(
              std::vector<double>{preStepMomentum[0], preStepMomentum[1],
                                  preStepMomentum[2], preStepEnergy});
          vertex->add_attribute("InitialParametersOf-" + trackId, preMom4);
        }
      }
    }
    if (track->GetCreatorProcess() != nullptr) {
      postParticle->add_attribute(
          "CreatorProcessOf-" + trackId,
          std::make_shared<::HepMC3::StringAttribute>(
              track->GetCreatorProcess()->GetProcessName()));
    }
  } else {
    // Add particle from same track to vertex
    m_previousVertex->add_particle_out(postParticle);
    m_previousVertex->add_attribute("NextProcessOf-" + trackId, process);
  }

  // Build the end vertex
  auto* postStep = step->GetPostStepPoint();
  auto postPosition = postStep->GetPosition() * convertLength;
  auto postTime = postStep->GetGlobalTime() * convertTime;
  ::HepMC3::FourVector postPos{postPosition[0], postPosition[1],
                               postPosition[2], postTime};
  m_previousVertex = std::make_shared<::HepMC3::GenVertex>(postPos);

  // Add particle to the vertex
  m_previousVertex->add_particle_in(postParticle);

  // Store the vertex
  event.add_vertex(m_previousVertex);

  // Store additional data in the particle
  postParticle->add_attribute(
      "TrackID", std::make_shared<::HepMC3::IntAttribute>(track->GetTrackID()));
  postParticle->add_attribute(
      "ParentID",
      std::make_shared<::HepMC3::IntAttribute>(track->GetParentID()));
  const double X0 = track->GetMaterial()->GetRadlen() * convertLength;
  const double L0 =
      track->GetMaterial()->GetNuclearInterLength() * convertLength;
  const double stepLength = track->GetStepLength();
  postParticle->add_attribute("NextX0",
                              std::make_shared<::HepMC3::DoubleAttribute>(X0));
  postParticle->add_attribute("NextL0",
                              std::make_shared<::HepMC3::DoubleAttribute>(L0));
  postParticle->add_attribute(
      "StepLength", std::make_shared<::HepMC3::DoubleAttribute>(stepLength));
  postParticle->set_status(1);

  // Stop tracking the vertex if the particle dies
  if (track->GetTrackStatus() != fAlive) {
    process = std::make_shared<::HepMC3::StringAttribute>("Death");
    m_previousVertex->add_attribute("NextProcessOf-" + trackId, process);
    m_previousVertex = nullptr;
  }
}

void SteppingAction::clear() {
  m_previousVertex = nullptr;
  m_eventAborted = false;
}

}  // namespace ActsExamples::Geant4::HepMC3
