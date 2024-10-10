// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ParticlesPrinter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/EventData/ProcessType.hpp"

#include <ostream>
#include <stdexcept>
#include <utility>

ActsExamples::ParticlesPrinter::ParticlesPrinter(const Config& cfg,
                                                 Acts::Logging::Level lvl)
    : IAlgorithm("ParticlesPrinter", lvl), m_cfg(cfg) {
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Input particles collection is not configured");
  }

  m_inputParticles.initialize(m_cfg.inputParticles);
}

ActsExamples::ProcessCode ActsExamples::ParticlesPrinter::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  using namespace Acts::UnitLiterals;

  const auto& particles = m_inputParticles(ctx);

  ACTS_INFO("event " << ctx.eventNumber << " collection '"
                     << m_cfg.inputParticles << "' contains "
                     << particles.size() << " particles");
  for (const auto& particle : particles) {
    ACTS_INFO("  particle " << particle);
    ACTS_INFO("    process_type: " << particle.process());
    ACTS_INFO("    position:     " << particle.position().transpose() / 1_mm
                                   << " mm");
    ACTS_INFO("    direction:    " << particle.direction().transpose());
    ACTS_INFO("    time:         " << particle.time() / 1_ns << " ns");
    ACTS_INFO("    |p|:          " << particle.absoluteMomentum() / 1_GeV
                                   << " GeV");
  }
  return ProcessCode::SUCCESS;
}
