// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/HepMC3/HepMC3Event.hpp"

#include "ActsExamples/Io/HepMC3/HepMC3Particle.hpp"
#include "ActsExamples/Io/HepMC3/HepMC3Vertex.hpp"

namespace ActsExamples {

namespace {

/// @brief Converts an SimParticle into HepMC3::GenParticle
/// @note The conversion ignores HepMC status codes
/// @param actsParticle Acts particle that will be converted
/// @return converted particle
HepMC3::GenParticlePtr actsParticleToGen(
    const std::shared_ptr<SimParticle>& actsParticle) {
  // Extract momentum and energy from Acts particle for HepMC3::FourVector
  const auto mom4 = actsParticle->fourMomentum();
  const HepMC3::FourVector vec(mom4[0], mom4[1], mom4[2], mom4[3]);
  // Create HepMC3::GenParticle
  auto genParticle =
      std::make_shared<HepMC3::GenParticle>(vec, actsParticle->pdg());
  genParticle->set_generated_mass(actsParticle->mass());

  return genParticle;
}

/// @brief Converts an Acts vertex to a HepMC3::GenVertexPtr
/// @note The conversion ignores HepMC status codes
/// @param actsVertex Acts vertex that will be converted
/// @return Converted Acts vertex to HepMC3::GenVertexPtr
HepMC3::GenVertexPtr createGenVertex(
    const std::shared_ptr<SimVertex>& actsVertex) {
  const HepMC3::FourVector vec(
      actsVertex->position4[0], actsVertex->position4[1],
      actsVertex->position4[2], actsVertex->position4[3]);

  // Create vertex
  auto genVertex = std::make_shared<HepMC3::GenVertex>(vec);

  return genVertex;
}

/// @brief Compares an Acts vertex with a HepMC3::GenVertex
/// @note An Acts vertex does not store a barcode. Therefore the content of
/// both vertices is compared. The position, time and number of incoming and
/// outgoing particles will be compared. Since a second vertex could exist in
/// the record with identical information (although unlikely), this
/// comparison could lead to false positive results. On the other hand, a
/// numerical deviation of the parameters could lead to a false negative.
/// @param actsVertex Acts vertex
/// @param genVertex HepMC3::GenVertex
/// @return boolean result if both vertices are identical
bool compareVertices(const std::shared_ptr<SimVertex>& actsVertex,
                     const HepMC3::GenVertexPtr& genVertex) {
  // Compare position, time, number of incoming and outgoing particles between
  // both vertices. Return false if one criterium does not match, else true.
  HepMC3::FourVector genVec = genVertex->position();
  if (actsVertex->position4[0] != genVec.x()) {
    return false;
  }
  if (actsVertex->position4[1] != genVec.y()) {
    return false;
  }
  if (actsVertex->position4[2] != genVec.z()) {
    return false;
  }
  if (actsVertex->position4[3] != genVec.t()) {
    return false;
  }
  if (actsVertex->incoming.size() != genVertex->particles_in().size()) {
    return false;
  }
  if (actsVertex->outgoing.size() != genVertex->particles_out().size()) {
    return false;
  }
  return true;
}
}  // namespace

///
/// Setter
///

void HepMC3Event::momentumUnit(HepMC3::GenEvent& event,
                               const double momentumUnit) {
  // Check, if the momentum unit fits Acts::UnitConstants::MeV or _GeV
  HepMC3::Units::MomentumUnit mom = HepMC3::Units::MomentumUnit::GEV;
  if (momentumUnit == Acts::UnitConstants::MeV) {
    mom = HepMC3::Units::MomentumUnit::MEV;
  } else if (momentumUnit == Acts::UnitConstants::GeV) {
    mom = HepMC3::Units::MomentumUnit::GEV;
  } else {
    // Report invalid momentum unit and set GeV
    std::cout << "Invalid unit of momentum: " << momentumUnit << std::endl;
    std::cout << "Momentum unit [GeV] will be used instead" << std::endl;
  }
  // Set units
  event.set_units(mom, event.length_unit());
}

void HepMC3Event::lengthUnit(HepMC3::GenEvent& event, const double lengthUnit) {
  // Check, if the length unit fits Acts::UnitConstants::mm or _cm
  HepMC3::Units::LengthUnit len = HepMC3::Units::LengthUnit::MM;
  if (lengthUnit == Acts::UnitConstants::mm) {
    len = HepMC3::Units::LengthUnit::MM;
  } else if (lengthUnit == Acts::UnitConstants::cm) {
    len = HepMC3::Units::LengthUnit::CM;
  } else {
    // Report invalid length unit and set mm
    std::cout << "Invalid unit of length: " << lengthUnit << std::endl;
    std::cout << "Length unit [mm] will be used instead" << std::endl;
  }

  // Set units
  event.set_units(event.momentum_unit(), len);
}

void HepMC3Event::shiftPositionBy(HepMC3::GenEvent& event,
                                  const Acts::Vector3& deltaPos,
                                  const double deltaTime) {
  // Create HepMC3::FourVector from position and time for shift
  const HepMC3::FourVector vec(deltaPos(0), deltaPos(1), deltaPos(2),
                               deltaTime);
  event.shift_position_by(vec);
}

void HepMC3Event::shiftPositionTo(HepMC3::GenEvent& event,
                                  const Acts::Vector3& pos, const double time) {
  // Create HepMC3::FourVector from position and time for the new position
  const HepMC3::FourVector vec(pos(0), pos(1), pos(2), time);
  event.shift_position_to(vec);
}

void HepMC3Event::shiftPositionTo(HepMC3::GenEvent& event,
                                  const Acts::Vector3& pos) {
  // Create HepMC3::FourVector from position and time for the new position
  const HepMC3::FourVector vec(pos(0), pos(1), pos(2), event.event_pos().t());
  event.shift_position_to(vec);
}

void HepMC3Event::shiftPositionTo(HepMC3::GenEvent& event, const double time) {
  // Create HepMC3::FourVector from position and time for the new position
  const HepMC3::FourVector vec(event.event_pos().x(), event.event_pos().y(),
                               event.event_pos().z(), time);
  event.shift_position_to(vec);
}

///
/// Adder
///

void HepMC3Event::addParticle(HepMC3::GenEvent& event,
                              const std::shared_ptr<SimParticle>& particle) {
  // Add new particle
  event.add_particle(actsParticleToGen(particle));
}

void HepMC3Event::addVertex(HepMC3::GenEvent& event,
                            const std::shared_ptr<SimVertex>& vertex) {
  // Add new vertex
  event.add_vertex(createGenVertex(vertex));
}

///
/// Remover
///

void HepMC3Event::removeParticle(HepMC3::GenEvent& event,
                                 const std::shared_ptr<SimParticle>& particle) {
  const std::vector<HepMC3::GenParticlePtr> genParticles = event.particles();
  const auto id = particle->particleId();
  // Search HepMC3::GenParticle with the same id as the Acts particle
  for (auto& genParticle : genParticles) {
    if (genParticle->id() == id) {
      // Remove particle if found
      event.remove_particle(genParticle);
      break;
    }
  }
}

void HepMC3Event::removeVertex(HepMC3::GenEvent& event,
                               const std::shared_ptr<SimVertex>& vertex) {
  const std::vector<HepMC3::GenVertexPtr> genVertices = event.vertices();
  // Walk over every recorded vertex
  for (auto& genVertex : genVertices) {
    if (compareVertices(vertex, genVertex)) {
      // Remove vertex if it matches actsVertex
      event.remove_vertex(genVertex);
      break;
    }
  }
}

///
/// Getter
///

double HepMC3Event::momentumUnit(const HepMC3::GenEvent& event) {
  // HepMC allows only MEV and GEV. This allows an easy identification.
  return (event.momentum_unit() == HepMC3::Units::MomentumUnit::MEV
              ? Acts::UnitConstants::MeV
              : Acts::UnitConstants::GeV);
}

double HepMC3Event::lengthUnit(const HepMC3::GenEvent& event) {
  // HepMC allows only MM and CM. This allows an easy identification.
  return (event.length_unit() == HepMC3::Units::LengthUnit::MM
              ? Acts::UnitConstants::mm
              : Acts::UnitConstants::cm);
}

Acts::Vector3 HepMC3Event::eventPos(const HepMC3::GenEvent& event) {
  // Extract the position from HepMC3::FourVector
  Acts::Vector3 vec;
  vec(0) = event.event_pos().x();
  vec(1) = event.event_pos().y();
  vec(2) = event.event_pos().z();
  return vec;
}

double HepMC3Event::eventTime(const HepMC3::GenEvent& event) {
  // Extract the time from HepMC3::FourVector
  return event.event_pos().t();
}

std::vector<SimParticle> HepMC3Event::particles(const HepMC3::GenEvent& event) {
  std::vector<SimParticle> actsParticles;
  const std::vector<HepMC3::ConstGenParticlePtr> genParticles =
      event.particles();

  // Translate all particles
  for (auto& genParticle : genParticles) {
    actsParticles.push_back(HepMC3Particle::particle(
        std::make_shared<HepMC3::GenParticle>(*genParticle)));
  }

  return actsParticles;
}

std::vector<std::unique_ptr<SimVertex>> HepMC3Event::vertices(
    const HepMC3::GenEvent& event) {
  std::vector<std::unique_ptr<SimVertex>> actsVertices;
  const std::vector<HepMC3::ConstGenVertexPtr> genVertices = event.vertices();

  // Translate all vertices
  for (auto& genVertex : genVertices) {
    actsVertices.push_back(HepMC3Vertex::processVertex(
        std::make_shared<HepMC3::GenVertex>(*genVertex)));
  }
  return actsVertices;
}

std::vector<SimParticle> HepMC3Event::beams(const HepMC3::GenEvent& event) {
  std::vector<SimParticle> actsBeams;
  const std::vector<HepMC3::ConstGenParticlePtr> genBeams = event.beams();

  // Translate beam particles and store the result
  for (auto& genBeam : genBeams) {
    actsBeams.push_back(HepMC3Particle::particle(
        std::make_shared<HepMC3::GenParticle>(*genBeam)));
  }
  return actsBeams;
}

std::vector<SimParticle> HepMC3Event::finalState(
    const HepMC3::GenEvent& event) {
  std::vector<HepMC3::ConstGenParticlePtr> particles = event.particles();
  std::vector<SimParticle> fState;

  // Walk over every vertex
  for (auto& particle : particles) {
    // Collect particles without end vertex
    if (!particle->end_vertex()) {
      fState.push_back(HepMC3Particle::particle(
          std::make_shared<HepMC3::GenParticle>(*particle)));
    }
  }
  return fState;
}

}  // namespace ActsExamples
