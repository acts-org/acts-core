// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "EventAction.hpp"

#include "Acts/Utilities/Helpers.hpp"

#include <stdexcept>

#include <G4Event.hh>
#include <G4RunManager.hh>

#include "SteppingAction.hpp"

namespace {

/// @brief This function tests whether a process is available in the record
///
/// @param [in] vertex The vertex that will be tested
/// @param [in] processFilter List of processes that will be filtered
///
/// @return True if the process was found, false if not
bool findAttribute(const HepMC3::ConstGenVertexPtr& vertex,
                   const std::vector<std::string>& processFilter) {
  // Consider only 1->1 vertices to keep a correct history
  if ((vertex->particles_in().size() == 1) &&
      (vertex->particles_out().size() == 1)) {
    // Test for all attributes if one matches the filter pattern
    const std::vector<std::string> vertexAttributes = vertex->attribute_names();
    for (const auto& att : vertexAttributes) {
      const std::string process = vertex->attribute_as_string(att);
      if (Acts::rangeContainsValue(processFilter, process)) {
        return true;
      }
    }
  }
  return false;
}

/// @brief This function reduces multiple vertices that should be filtered and
/// combines them in a single dummy vertex
///
/// @param [in, out] event The underlying event
/// @param [in, out] vertex The vertex that will be reduced
/// @param [in] processFilter List of processes that will be filtered
void reduceVertex(HepMC3::GenEvent& event, HepMC3::GenVertexPtr vertex,
                  const std::vector<std::string>& processFilter) {
  // Store the particles associated to the vertex
  HepMC3::GenParticlePtr particleIn = vertex->particles_in()[0];
  HepMC3::GenParticlePtr particleOut = vertex->particles_out()[0];

  // Walk backwards to find all vertices in a row that match the pattern
  while (findAttribute(particleIn->production_vertex(), processFilter)) {
    // Take the particles before the vertex and remove particles & vertices in
    // between
    HepMC3::GenVertexPtr currentVertex = particleOut->production_vertex();
    HepMC3::GenParticlePtr nextParticle = currentVertex->particles_in()[0];
    // Cut connections from particle and remove it
    if (particleIn->end_vertex()) {
      particleIn->end_vertex()->remove_particle_in(particleIn);
    }
    currentVertex->remove_particle_out(particleIn);
    event.remove_particle(particleIn);
    // Cut connections from vertext and remove it
    particleIn = nextParticle;
    currentVertex->remove_particle_in(particleIn);
    event.remove_vertex(currentVertex);
  }
  // Walk forwards to find all vertices in a row that match the pattern
  while (findAttribute(particleOut->end_vertex(), processFilter)) {
    // Take the particles after the vertex and remove particles & vertices in
    // between
    HepMC3::GenVertexPtr currentVertex = particleOut->end_vertex();
    HepMC3::GenParticlePtr nextParticle = currentVertex->particles_out()[0];
    // Cut connections from particle and remove it
    if (particleOut->production_vertex()) {
      particleOut->production_vertex()->remove_particle_out(particleOut);
    }
    currentVertex->remove_particle_in(particleOut);
    event.remove_particle(particleOut);
    // Cut connections from vertext and remove it
    particleOut = nextParticle;
    currentVertex->remove_particle_out(particleOut);
    event.remove_vertex(currentVertex);
  }

  // Build the dummy vertex
  auto reducedVertex = std::make_shared<HepMC3::GenVertex>();
  event.add_vertex(reducedVertex);

  reducedVertex->add_particle_in(particleIn);
  reducedVertex->add_particle_out(particleOut);

  // Remove and replace the old vertex
  event.remove_vertex(vertex);
  vertex = reducedVertex;
}

/// @brief This method walks over all vertices and tests whether it should be
/// filtered
///
/// @param [in, out] event The underlying event
/// @param [in, out] The current vertex under investigation
/// @param [in] processFilter List of processes that will be filtered
void followOutgoingParticles(HepMC3::GenEvent& event,
                             const HepMC3::GenVertexPtr& vertex,
                             const std::vector<std::string>& processFilter) {
  // Replace and reduce vertex if it should be filtered
  if (findAttribute(vertex, processFilter)) {
    reduceVertex(event, vertex, processFilter);
  }
  // Move forward to the next vertices
  for (const auto& particle : vertex->particles_out()) {
    followOutgoingParticles(event, particle->end_vertex(), processFilter);
  }
}
}  // namespace

namespace ActsExamples::Geant4::HepMC3 {

EventAction* EventAction::s_instance = nullptr;

EventAction* EventAction::instance() {
  // Static access function via G4RunManager
  return s_instance;
}

EventAction::EventAction(std::vector<std::string> processFilter)
    : G4UserEventAction(), m_processFilter(std::move(processFilter)) {
  if (s_instance != nullptr) {
    throw std::logic_error("Attempted to duplicate a singleton");
  } else {
    s_instance = this;
  }
}

EventAction::~EventAction() {
  s_instance = nullptr;
}

void EventAction::BeginOfEventAction(const G4Event* /*event*/) {
  SteppingAction::instance()->clear();
  m_event = ::HepMC3::GenEvent(::HepMC3::Units::GEV, ::HepMC3::Units::MM);
  m_event.add_beam_particle(std::make_shared<::HepMC3::GenParticle>());
}

void EventAction::EndOfEventAction(const G4Event* /*event*/) {
  // Fast exit if the event is empty
  if (m_event.vertices().empty()) {
    return;
  }
  // Filter irrelevant processes
  auto currentVertex = m_event.vertices()[0];
  for (auto& bp : m_event.beams()) {
    if (!bp->end_vertex()) {
      currentVertex->add_particle_in(bp);
    }
  }
  followOutgoingParticles(m_event, currentVertex, m_processFilter);
  // Remove vertices w/o outgoing particles and particles w/o production
  // vertices
  while (true) {
    bool sane = true;
    for (const auto& v : m_event.vertices()) {
      if (!v) {
        continue;
      }
      if (v->particles_out().empty()) {
        m_event.remove_vertex(v);
        sane = false;
      }
    }
    for (const auto& p : m_event.particles()) {
      if (!p) {
        continue;
      }
      if (!p->production_vertex()) {
        m_event.remove_particle(p);
        sane = false;
      }
    }
    if (sane) {
      break;
    }
  }
}

void EventAction::clear() {
  SteppingAction::instance()->clear();
}

::HepMC3::GenEvent& EventAction::event() {
  return m_event;
}
}  // namespace ActsExamples::Geant4::HepMC3
