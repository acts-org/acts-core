// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TruthTracking/HitSelector.hpp"

#include "Acts/Utilities/MathHelpers.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"

namespace ActsExamples {

HitSelector::HitSelector(const Config& config, Acts::Logging::Level level)
    : IAlgorithm("HitSelector", level), m_cfg(config) {
  if (m_cfg.minX >= m_cfg.maxX || m_cfg.minY >= m_cfg.maxY ||
      m_cfg.minZ >= m_cfg.maxZ || m_cfg.minR >= m_cfg.maxR ||
      m_cfg.minTime >= m_cfg.maxTime ||
      m_cfg.minEnergyLoss >= m_cfg.maxEnergyLoss ||
      m_cfg.minPrimaryVertexId >= m_cfg.maxPrimaryVertexId) {
    throw std::invalid_argument(
        "Invalid bounds configuration: min values must be less than max "
        "values");
  }
  m_inputHits.initialize(m_cfg.inputHits);
  m_inputParticlesSelected.maybeInitialize(m_cfg.inputParticlesSelected);
  m_outputHits.initialize(m_cfg.outputHits);

  ACTS_DEBUG("selection particles " << m_cfg.inputParticlesSelected);
  ACTS_DEBUG("selection hit x [" << m_cfg.minX << "," << m_cfg.maxX << ")");
  ACTS_DEBUG("selection hit y [" << m_cfg.minY << "," << m_cfg.maxY << ")");
  ACTS_DEBUG("selection hit z [" << m_cfg.minZ << "," << m_cfg.maxZ << ")");
  ACTS_DEBUG("selection hit r [" << m_cfg.minR << "," << m_cfg.maxR << ")");
  ACTS_DEBUG("selection hit time [" << m_cfg.minTime << "," << m_cfg.maxTime
                                    << ")");
  ACTS_DEBUG("selection hit energy loss [" << m_cfg.minEnergyLoss << ","
                                           << m_cfg.maxEnergyLoss << ")");
  ACTS_DEBUG("selection primary vertex ID [" << m_cfg.minPrimaryVertexId << ","
                                             << m_cfg.maxPrimaryVertexId
                                             << ")");
}

ProcessCode HitSelector::execute(const AlgorithmContext& ctx) const {
  const SimHitContainer& hits = m_inputHits(ctx);
  const SimParticleContainer* particlesSelected =
      m_inputParticlesSelected.isInitialized() ? &m_inputParticlesSelected(ctx)
                                               : nullptr;

  std::vector<SimHit> unorderedHits;
  unorderedHits.reserve(hits.size());

  for (const auto& hit : hits) {
    const double r = Acts::fastHypot(hit.position().x(), hit.position().y());
    const std::uint64_t primaryVertexId = hit.particleId().vertexPrimary();

    const bool validParticle = (particlesSelected == nullptr) ||
                               particlesSelected->contains(hit.particleId());
    const bool validX =
        (m_cfg.minX <= hit.position().x()) && (hit.position().x() < m_cfg.maxX);
    const bool validY =
        (m_cfg.minY <= hit.position().y()) && (hit.position().y() < m_cfg.maxY);
    const bool validZ =
        (m_cfg.minZ <= hit.position().z()) && (hit.position().z() < m_cfg.maxZ);
    const bool validR = (m_cfg.minR <= r) && (r < m_cfg.maxR);
    const bool validTime =
        (m_cfg.minTime <= hit.time()) && (hit.time() < m_cfg.maxTime);
    const bool validEnergyLoss =
        (m_cfg.minEnergyLoss <= hit.depositedEnergy()) &&
        (hit.depositedEnergy() < m_cfg.maxEnergyLoss);
    const bool validPrimaryVertexId =
        (m_cfg.minPrimaryVertexId <= primaryVertexId) &&
        (primaryVertexId < m_cfg.maxPrimaryVertexId);

    const bool validHit = validParticle && validX && validY && validZ &&
                          validR && validTime && validEnergyLoss &&
                          validPrimaryVertexId;
    if (validHit) {
      unorderedHits.push_back(hit);
    }
  }

  // hits are still sorted after filtering
  SimHitContainer selectedHits(boost::container::ordered_range_t{},
                               unorderedHits.begin(), unorderedHits.end());

  ACTS_DEBUG("selected " << selectedHits.size() << " from " << hits.size()
                         << " hits");

  m_outputHits(ctx, std::move(selectedHits));

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
