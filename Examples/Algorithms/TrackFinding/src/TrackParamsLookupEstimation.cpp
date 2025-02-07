// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TrackFinding/TrackParamsLookupEstimation.hpp"

#include "Acts/Surfaces/RectangleBounds.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

ActsExamples::TrackParamsLookupEstimation::TrackParamsLookupEstimation(
    const Config& config, Acts::Logging::Level level)
    : IAlgorithm("TrackParamsLookupEstimation", level), m_cfg(config) {
  // Iterate over the reference layers and create
  // track parameter accumulators
  for (const auto& [geoId, refSurface] : m_cfg.refLayers) {
    // Get bounds to construct the accumulator grid
    auto bounds =
        dynamic_cast<const Acts::RectangleBounds*>(&refSurface->bounds());

    if (bounds == nullptr) {
      throw std::invalid_argument("Only rectangle bounds supported");
    }
    if (refSurface->type() != Acts::Surface::SurfaceType::Plane) {
      throw std::invalid_argument("Only plane surfaces supported");
    }

    // Initialize the accumulator grid
    auto halfX = bounds->halfLengthX();
    auto halfY = bounds->halfLengthY();

    TrackParamsLookupAxisGen axisGen{
        {-halfX, halfX}, m_cfg.bins.first, {-halfY, halfY}, m_cfg.bins.second};

    // Each reference layer has its own accumulator
    m_accumulators[geoId] = std::make_unique<TrackParamsLookupAccumulator>(
        TrackParamsLookupGrid(axisGen()));
  }

  m_inputParticles.initialize(m_cfg.inputParticles);
  m_inputSimHits.initialize(m_cfg.inputHits);
}

ActsExamples::ProcessCode
ActsExamples::TrackParamsLookupEstimation::finalize() {
  // Finiliaze the lookup tables and write them
  ActsExamples::TrackParamsLookup lookup;
  for (auto& [id, acc] : m_accumulators) {
    lookup.insert({id, acc->finalizeLookup()});
  }
  for (const auto& writer : m_cfg.trackLookupGridWriters) {
    writer->writeLookup(lookup);
  }

  return ActsExamples::ProcessCode::SUCCESS;
};

ActsExamples::ProcessCode ActsExamples::TrackParamsLookupEstimation::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  // Get the particles and hits
  const auto& particles = m_inputParticles(ctx);
  const auto& hits = m_inputSimHits(ctx);

  // Iterate over the reference layer hits and
  // accumulate the track parameters
  for (const auto& [geoId, refSurface] : m_cfg.refLayers) {
    // Get reference layer hits
    auto refLayerHits = hits.equal_range(geoId);

    for (auto hit = refLayerHits.first; hit != refLayerHits.second; ++hit) {
      // Get the corresponding particle
      const auto& id = hit->particleId();
      const auto& particle = particles.find(id);

      if (particle == particles.end()) {
        throw std::invalid_argument("Particle not found");
      }

      // Hit stores the reference layer parameters
      auto refLayerPars = Acts::CurvilinearTrackParameters(
          hit->fourPosition(), hit->direction(), particle->qOverP(),
          std::nullopt, particle->hypothesis());

      // Particle stores the IP parameters
      auto ipPars = Acts::CurvilinearTrackParameters(
          particle->fourPosition(), particle->direction(), particle->qOverP(),
          std::nullopt, particle->hypothesis());

      // Get the local position of the hit
      auto localPos = refSurface
                          ->globalToLocal(ctx.geoContext, hit->position(),
                                          Acts::Vector3{0, 1, 0})
                          .value();

      // Add the track parameters to the accumulator grid
      m_accumulators.at(geoId)->addTrack(ipPars, refLayerPars, localPos);
    }
  }

  return ActsExamples::ProcessCode::SUCCESS;
}
