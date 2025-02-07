// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ParticlesPrinter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsFatras/EventData/ProcessType.hpp"

#include <ostream>
#include <stdexcept>

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
