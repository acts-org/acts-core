// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/EDM4hep/EDM4hepSimHitWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepUtil.hpp"

#include <stdexcept>

#include <edm4hep/MCParticle.h>
#include <edm4hep/SimTrackerHit.h>
#include <podio/Frame.h>

namespace ActsExamples {

EDM4hepSimHitWriter::EDM4hepSimHitWriter(
    const EDM4hepSimHitWriter::Config& config, Acts::Logging::Level level)
    : WriterT(config.inputSimHits, "EDM4hepSimHitWriter", level),
      m_cfg(config),
      m_writer(config.outputPath) {
  ACTS_VERBOSE("Created output file " << config.outputPath);

  if (m_cfg.inputSimHits.empty()) {
    throw std::invalid_argument("Missing simulated hits input collection");
  }

  if (m_cfg.outputParticles.empty()) {
    throw std::invalid_argument("Missing output particle name");
  }

  if (m_cfg.outputSimTrackerHits.empty()) {
    throw std::invalid_argument("Missing output sim tracker hit name");
  }

  m_inputParticles.maybeInitialize(m_cfg.inputParticles);
}

ActsExamples::ProcessCode EDM4hepSimHitWriter::finalize() {
  m_writer.finish();

  return ProcessCode::SUCCESS;
}

ProcessCode EDM4hepSimHitWriter::writeT(const AlgorithmContext& ctx,
                                        const SimHitContainer& simHits) {
  podio::Frame frame;

  EDM4hepUtil::MapParticleIdTo particleMapper;
  std::unordered_map<ActsFatras::Barcode, edm4hep::MutableMCParticle>
      particleMap;

  edm4hep::MCParticleCollection mcParticles;
  edm4hep::SimTrackerHitCollection simTrackerHitCollection;

  if (!m_cfg.inputParticles.empty()) {
    auto particles = m_inputParticles(ctx);

    for (const auto& particle : particles) {
      auto p = mcParticles->create();
      particleMap[particle.particleId()] = p;
      EDM4hepUtil::writeParticle(particle, p);
    }

    particleMapper = [&](ActsFatras::Barcode particleId) {
      auto it = particleMap.find(particleId);
      if (it == particleMap.end()) {
        throw std::runtime_error("Particle not found in map");
      }
      return it->second;
    };
  }

  for (const auto& simHit : simHits) {
    auto simTrackerHit = simTrackerHitCollection->create();
    EDM4hepUtil::writeSimHit(
        simHit, simTrackerHit, particleMapper,
        [](Acts::GeometryIdentifier id) { return id.value(); });
  }

  frame.put(std::move(mcParticles), m_cfg.outputParticles);
  frame.put(std::move(simTrackerHitCollection), m_cfg.outputSimTrackerHits);

  std::lock_guard lock{m_writeMutex};
  m_writer.writeFrame(frame, "events");

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
