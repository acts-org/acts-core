// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TruthTracking/ParticleTrackParamExtractor.hpp"

#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <stdexcept>
#include <utility>

namespace ActsExamples {

ParticleTrackParamExtractor::ParticleTrackParamExtractor(
    const Config& config, Acts::Logging::Level level)
    : IAlgorithm("ParticleTrackParamExtractor", level), m_cfg(config) {
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Missing input particles collection");
  }
  if (m_cfg.outputTrackParameters.empty()) {
    throw std::invalid_argument("Missing output track parameters collection");
  }

  m_inputParticles.initialize(m_cfg.inputParticles);
  m_outputTrackParameters.initialize(m_cfg.outputTrackParameters);
}

ActsExamples::ProcessCode ParticleTrackParamExtractor::execute(
    const AlgorithmContext& ctx) const {
  const SimParticleContainer& particles = m_inputParticles(ctx);

  std::unordered_map<SimBarcode, std::shared_ptr<Acts::PerigeeSurface>>
      perigeeSurfaces;

  for (auto&& [vtxId, vtxParticles] : groupBySecondaryVertex(particles)) {
    // a group contains at least one particle by construction. assume that all
    // particles within the group originate from the same position and use it
    // to as the reference position for the perigee frame.
    auto perigee = Acts::Surface::makeShared<Acts::PerigeeSurface>(
        vtxParticles.begin()->position());
    perigeeSurfaces[vtxId] = perigee;
  }

  // create track parameters from the particles
  TrackParametersContainer trackParameters;

  for (const auto& particle : particles) {
    const auto vtxId = particle.particleId().vertexId();
    const auto particleHypothesis = particle.hypothesis();
    const auto phi = Acts::VectorHelpers::phi(particle.direction());
    const auto theta = Acts::VectorHelpers::theta(particle.direction());
    const auto qOverP = particle.qOverP();
    const auto time = particle.time();

    trackParameters.emplace_back(
        perigeeSurfaces.at(vtxId),
        Acts::BoundVector{0, 0, phi, theta, qOverP, time}, std::nullopt,
        particleHypothesis);
  }

  m_outputTrackParameters(ctx, std::move(trackParameters));

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
