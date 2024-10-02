// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TruthTracking/TruthSeedSelector.hpp"

#include "Acts/Utilities/MultiIndex.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Utilities/Range.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <functional>
#include <stdexcept>
#include <utility>

namespace ActsExamples {
struct AlgorithmContext;
}  // namespace ActsExamples

using namespace ActsExamples;

TruthSeedSelector::TruthSeedSelector(const Config& config,
                                     Acts::Logging::Level level)
    : IAlgorithm("TruthSeedSelector", level), m_cfg(config) {
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Missing input truth particles collection");
  }
  if (m_cfg.inputMeasurementParticlesMap.empty()) {
    throw std::invalid_argument("Missing input hit-particles map collection");
  }
  if (m_cfg.outputParticles.empty()) {
    throw std::invalid_argument("Missing output truth particles collection");
  }

  m_inputParticles.initialize(m_cfg.inputParticles);
  m_inputMeasurementParticlesMap.initialize(m_cfg.inputMeasurementParticlesMap);
  m_outputParticles.initialize(m_cfg.outputParticles);
}

ProcessCode TruthSeedSelector::execute(const AlgorithmContext& ctx) const {
  // prepare input collections
  const auto& inputParticles = m_inputParticles(ctx);
  const auto& hitParticlesMap = m_inputMeasurementParticlesMap(ctx);
  // compute particle_id -> {hit_id...} map from the
  // hit_id -> {particle_id...} map on the fly.
  const auto& particleHitsMap = invertIndexMultimap(hitParticlesMap);

  // prepare output collection
  SimParticleContainer selectedParticles;
  selectedParticles.reserve(inputParticles.size());

  auto within = [](double x, double min, double max) {
    return (min <= x) && (x < max);
  };
  auto isValidparticle = [&](const auto& p) {
    const auto eta = Acts::VectorHelpers::eta(p.direction());
    const auto phi = Acts::VectorHelpers::phi(p.direction());
    const auto rho = Acts::VectorHelpers::perp(p.position());
    // find the corresponding hits for this particle
    const auto& hits = makeRange(particleHitsMap.equal_range(p.particleId()));
    // number of recorded hits
    std::size_t nHits = hits.size();
    return within(rho, 0., m_cfg.rhoMax) &&
           within(p.position().z(), m_cfg.zMin, m_cfg.zMax) &&
           within(std::abs(eta), m_cfg.absEtaMin, m_cfg.absEtaMax) &&
           within(eta, m_cfg.etaMin, m_cfg.etaMax) &&
           within(phi, m_cfg.phiMin, m_cfg.phiMax) &&
           within(p.transverseMomentum(), m_cfg.ptMin, m_cfg.ptMax) &&
           within(nHits, m_cfg.nHitsMin, m_cfg.nHitsMax) &&
           (m_cfg.keepNeutral || (p.charge() != 0));
  };

  // create prototracks for all input particles
  for (const auto& particle : inputParticles) {
    if (isValidparticle(particle)) {
      selectedParticles.insert(particle);
    }
  }

  m_outputParticles(ctx, std::move(selectedParticles));
  return ProcessCode::SUCCESS;
}
