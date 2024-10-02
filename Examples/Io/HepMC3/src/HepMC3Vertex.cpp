// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/HepMC3/HepMC3Vertex.hpp"

#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimVertex.hpp"
#include "ActsExamples/Io/HepMC3/HepMC3Particle.hpp"

namespace ActsExamples {

namespace {

/// @brief Converts HepMC3::GenParticle objects into Acts
/// @param genParticles list of HepMC3::GenParticle objects
/// @return converted list
SimBarcodeContainer genBarcodeToActs(
    const std::vector<HepMC3::GenParticlePtr>& genParticles) {
  SimBarcodeContainer actsBarcodes;
  // Translate all particles
  for (auto& genParticle : genParticles) {
    actsBarcodes.insert(HepMC3Particle::barcode(
        std::make_shared<HepMC3::GenParticle>(*genParticle)));
  }
  return actsBarcodes;
}

/// @brief Converts HepMC3::GenParticle objects into Acts
/// @param genParticles list of HepMC3::GenParticle objects
/// @return converted list
std::vector<SimParticle> genParticlesToActs(
    const std::vector<HepMC3::GenParticlePtr>& genParticles) {
  std::vector<SimParticle> actsParticles;
  // Translate all particles
  for (auto& genParticle : genParticles) {
    actsParticles.push_back(HepMC3Particle::particle(
        std::make_shared<HepMC3::GenParticle>(*genParticle)));
  }
  return actsParticles;
}

/// @brief Converts an SimParticle into HepMC3::GenParticle
/// @note The conversion ignores HepMC status codes
/// @param actsParticle Acts particle that will be converted
/// @return converted particle
HepMC3::GenParticlePtr actsParticleToGen(
    const std::shared_ptr<SimParticle>& actsParticle) {
  // Extract momentum and energy from Acts particle for HepMC3::FourVector
  const auto mom = actsParticle->fourMomentum();
  const HepMC3::FourVector vec(mom[0], mom[1], mom[2], mom[3]);
  // Create HepMC3::GenParticle
  auto genParticle =
      std::make_shared<HepMC3::GenParticle>(vec, actsParticle->pdg());
  genParticle->set_generated_mass(actsParticle->mass());

  return genParticle;
}

/// @brief Finds a HepMC3::GenParticle from a list that matches an
/// SimParticle object
/// @param genParticles list of HepMC particles
/// @param actsParticle Acts particle
/// @return HepMC particle that matched with the Acts particle or nullptr if
/// no match was found
HepMC3::GenParticlePtr matchParticles(
    const std::vector<HepMC3::GenParticlePtr>& genParticles,
    const std::shared_ptr<SimParticle>& actsParticle) {
  const auto id = actsParticle->particleId();
  // Search HepMC3::GenParticle with the same id as the Acts particle
  for (auto& genParticle : genParticles) {
    if (genParticle->id() == id) {
      // Return particle if found
      return genParticle;
    }
  }
  return nullptr;
}

}  // namespace

std::unique_ptr<SimVertex> HepMC3Vertex::processVertex(
    const std::shared_ptr<HepMC3::GenVertex>& vertex) {
  SimVertex vtx(SimVertexBarcode().setVertexPrimary(vertex->id()),
                {vertex->position().x(), vertex->position().y(),
                 vertex->position().z(), vertex->position().t()});
  vtx.incoming = genBarcodeToActs(vertex->particles_in());
  vtx.outgoing = genBarcodeToActs(vertex->particles_out());
  // Create Acts vertex
  return std::make_unique<SimVertex>(std::move(vtx));
}

bool HepMC3Vertex::inEvent(const std::shared_ptr<HepMC3::GenVertex>& vertex) {
  return vertex->in_event();
}

int HepMC3Vertex::id(const std::shared_ptr<HepMC3::GenVertex>& vertex) {
  return vertex->id();
}

std::vector<SimParticle> HepMC3Vertex::particlesIn(
    const std::shared_ptr<HepMC3::GenVertex>& vertex) {
  return genParticlesToActs(vertex->particles_in());
}

std::vector<SimParticle> HepMC3Vertex::particlesOut(
    const std::shared_ptr<HepMC3::GenVertex>& vertex) {
  return genParticlesToActs(vertex->particles_out());
}

Acts::Vector3 HepMC3Vertex::position(
    const std::shared_ptr<HepMC3::GenVertex>& vertex) {
  Acts::Vector3 vec;
  vec(0) = vertex->position().x();
  vec(1) = vertex->position().y();
  vec(2) = vertex->position().z();
  return vec;
}

double HepMC3Vertex::time(const std::shared_ptr<HepMC3::GenVertex>& vertex) {
  return vertex->position().t();
}

void HepMC3Vertex::addParticleIn(
    const std::shared_ptr<HepMC3::GenVertex>& vertex,
    const std::shared_ptr<SimParticle>& particle) {
  vertex->add_particle_in(actsParticleToGen(particle));
}

void HepMC3Vertex::addParticleOut(
    const std::shared_ptr<HepMC3::GenVertex>& vertex,
    const std::shared_ptr<SimParticle>& particle) {
  vertex->add_particle_out(actsParticleToGen(particle));
}

void HepMC3Vertex::removeParticleIn(
    const std::shared_ptr<HepMC3::GenVertex>& vertex,
    const std::shared_ptr<SimParticle>& particle) {
  // Remove particle if it exists
  if (HepMC3::GenParticlePtr genParticle =
          matchParticles(vertex->particles_in(), particle)) {
    vertex->remove_particle_in(genParticle);
  }
}

void HepMC3Vertex::removeParticleOut(
    const std::shared_ptr<HepMC3::GenVertex>& vertex,
    const std::shared_ptr<SimParticle>& particle) {
  // Remove particle if it exists
  if (HepMC3::GenParticlePtr genParticle =
          matchParticles(vertex->particles_out(), particle)) {
    vertex->remove_particle_out(genParticle);
  }
}

void HepMC3Vertex::position(const std::shared_ptr<HepMC3::GenVertex>& vertex,
                            const Acts::Vector3& pos) {
  HepMC3::FourVector fVec(pos(0), pos(1), pos(2), vertex->position().t());
  vertex->set_position(fVec);
}

void HepMC3Vertex::time(const std::shared_ptr<HepMC3::GenVertex>& vertex,
                        double time) {
  HepMC3::FourVector fVec(vertex->position().x(), vertex->position().y(),
                          vertex->position().z(), time);
  vertex->set_position(fVec);
}

}  // namespace ActsExamples
