// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Generators/EventGenerator.hpp"

#include "ActsExamples/EventData/SimVertex.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <limits>
#include <ostream>
#include <stdexcept>

ActsExamples::EventGenerator::EventGenerator(const Config& cfg,
                                             Acts::Logging::Level lvl)
    : m_cfg(cfg), m_logger(Acts::getDefaultLogger("EventGenerator", lvl)) {
  if (m_cfg.outputParticles.empty()) {
    throw std::invalid_argument("Missing output particles collection");
  }
  if (m_cfg.outputVertices.empty()) {
    throw std::invalid_argument("Missing output vertices collection");
  }
  if (m_cfg.generators.empty()) {
    throw std::invalid_argument("No generators are configured");
  }
  if (!m_cfg.randomNumbers) {
    throw std::invalid_argument("Missing random numbers service");
  }

  m_outputParticles.initialize(m_cfg.outputParticles);
  m_outputVertices.initialize(m_cfg.outputVertices);
}

std::string ActsExamples::EventGenerator::name() const {
  return "EventGenerator";
}

std::pair<std::size_t, std::size_t>
ActsExamples::EventGenerator::availableEvents() const {
  return {0u, std::numeric_limits<std::size_t>::max()};
}

ActsExamples::ProcessCode ActsExamples::EventGenerator::read(
    const AlgorithmContext& ctx) {
  SimParticleContainer particles;
  SimVertexContainer vertices;

  auto rng = m_cfg.randomNumbers->spawnGenerator(ctx);

  std::size_t nPrimaryVertices = 0;
  for (std::size_t iGenerate = 0; iGenerate < m_cfg.generators.size();
       ++iGenerate) {
    auto& generate = m_cfg.generators[iGenerate];

    // generate the primary vertices from this generator
    for (std::size_t n = (*generate.multiplicity)(rng); 0 < n; --n) {
      nPrimaryVertices += 1;

      // generate primary vertex position
      auto vertexPosition = (*generate.vertex)(rng);
      // generate particles associated to this vertex
      auto [newVertices, newParticles] = (*generate.particles)(rng);

      ACTS_VERBOSE("Generate vertex at " << vertexPosition.transpose());

      auto updateParticleInPlace = [&](SimParticle& particle) {
        // only set the primary vertex, leave everything else as-is
        // using the number of primary vertices as the index ensures
        // that barcode=0 is not used, since it is used elsewhere
        // to signify elements w/o an associated particle.
        const auto pid = SimBarcode{particle.particleId()}.setVertexPrimary(
            nPrimaryVertices);
        // move particle to the vertex
        const auto pos4 = (vertexPosition + particle.fourPosition()).eval();
        ACTS_VERBOSE(" - particle at " << pos4.transpose());
        // `withParticleId` returns a copy because it changes the identity
        particle = particle.withParticleId(pid).setPosition4(pos4);
      };
      for (auto& vertexParticle : newParticles) {
        updateParticleInPlace(vertexParticle);
      }

      auto updateVertexInPlace = [&](SimVertex& vertex) {
        // only set the primary vertex, leave everything else as-is
        // using the number of primary vertices as the index ensures
        // that barcode=0 is not used, since it is used elsewhere
        // to signify elements w/o an associated particle.
        vertex.id = SimVertexBarcode{vertex.vertexId()}.setVertexPrimary(
            nPrimaryVertices);
        // move vertex
        const auto pos4 = (vertexPosition + vertex.position4).eval();
        ACTS_VERBOSE(" - vertex at " << pos4.transpose());
        vertex.position4 = pos4;
      };
      for (auto& vertex : newVertices) {
        updateVertexInPlace(vertex);
      }

      ACTS_VERBOSE("event=" << ctx.eventNumber << " generator=" << iGenerate
                            << " primary_vertex=" << nPrimaryVertices
                            << " n_particles=" << newParticles.size());

      particles.merge(std::move(newParticles));
      vertices.merge(std::move(newVertices));
    }
  }

  ACTS_DEBUG("event=" << ctx.eventNumber
                      << " n_primary_vertices=" << nPrimaryVertices
                      << " n_particles=" << particles.size());

  // move generated event to the store
  m_outputParticles(ctx, std::move(particles));
  m_outputVertices(ctx, std::move(vertices));
  return ProcessCode::SUCCESS;
}
