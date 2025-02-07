// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TruthTracking/TruthTrackFinder.hpp"

#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Utilities/Range.hpp"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace ActsExamples {

TruthTrackFinder::TruthTrackFinder(const Config& config,
                                   Acts::Logging::Level level)
    : IAlgorithm("TruthTrackFinder", level), m_cfg(config) {
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Missing input truth particles collection");
  }
  if (m_cfg.inputParticleMeasurementsMap.empty()) {
    throw std::invalid_argument("Missing input hit-particles map collection");
  }
  if (m_cfg.outputProtoTracks.empty()) {
    throw std::invalid_argument("Missing output proto tracks collection");
  }

  m_inputParticles.initialize(m_cfg.inputParticles);
  m_inputParticleMeasurementsMap.initialize(m_cfg.inputParticleMeasurementsMap);
  m_outputProtoTracks.initialize(m_cfg.outputProtoTracks);
}

ProcessCode TruthTrackFinder::execute(const AlgorithmContext& ctx) const {
  // prepare input collections
  const auto& particles = m_inputParticles(ctx);
  const auto& particleMeasurementsMap = m_inputParticleMeasurementsMap(ctx);

  // prepare output collection
  ProtoTrackContainer tracks;
  tracks.reserve(particles.size());

  ACTS_VERBOSE("Create prototracks for " << particles.size() << " particles");
  for (const auto& particle : particles) {
    // find the corresponding hits for this particle
    const auto& measurements =
        makeRange(particleMeasurementsMap.equal_range(particle.particleId()));
    ACTS_VERBOSE(" - Prototrack from " << measurements.size()
                                       << " measurements");
    // fill hit indices to create the proto track
    ProtoTrack track;
    track.reserve(measurements.size());
    for (const auto& measurement : measurements) {
      track.emplace_back(measurement.second);
    }
    // add proto track to the output collection
    tracks.emplace_back(std::move(track));
  }

  m_outputProtoTracks(ctx, std::move(tracks));
  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
