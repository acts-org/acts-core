// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TruthTracking/TruthSeedingAlgorithm.hpp"

#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Utilities/MultiIndex.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Utilities/Range.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace ActsExamples {
struct AlgorithmContext;
}  // namespace ActsExamples

ActsExamples::TruthSeedingAlgorithm::TruthSeedingAlgorithm(
    ActsExamples::TruthSeedingAlgorithm::Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("TruthSeedingAlgorithm", lvl),
      m_cfg(std::move(cfg)) {
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Missing input truth particles collection");
  }
  if (m_cfg.inputMeasurementParticlesMap.empty()) {
    throw std::invalid_argument("Missing input hit-particles map collection");
  }
  if (m_cfg.inputSpacePoints.empty()) {
    throw std::invalid_argument("Missing seeds or space point collection");
  }

  for (const auto& spName : m_cfg.inputSpacePoints) {
    if (spName.empty()) {
      throw std::invalid_argument("Invalid space point input collection");
    }

    auto& handle = m_inputSpacePoints.emplace_back(
        std::make_unique<ReadDataHandle<SimSpacePointContainer>>(
            this,
            "InputSpacePoints#" + std::to_string(m_inputSpacePoints.size())));
    handle->initialize(spName);
  }

  if (m_cfg.outputParticles.empty()) {
    throw std::invalid_argument("Missing output particles collection");
  }
  if (m_cfg.outputSeeds.empty()) {
    throw std::invalid_argument("Missing seeds output collections");
  }
  if (m_cfg.outputProtoTracks.empty()) {
    throw std::invalid_argument("Missing proto tracks output collections");
  }

  m_inputParticles.initialize(m_cfg.inputParticles);
  m_inputMeasurementParticlesMap.initialize(m_cfg.inputMeasurementParticlesMap);
  m_outputParticles.initialize(m_cfg.outputParticles);
  m_outputProtoTracks.initialize(m_cfg.outputProtoTracks);
  m_outputSeeds.initialize(m_cfg.outputSeeds);
}

ActsExamples::ProcessCode ActsExamples::TruthSeedingAlgorithm::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  // prepare input collections
  const auto& particles = m_inputParticles(ctx);
  const auto& hitParticlesMap = m_inputMeasurementParticlesMap(ctx);
  // compute particle_id -> {hit_id...} map from the
  // hit_id -> {particle_id...} map on the fly.
  const auto& particleHitsMap = invertIndexMultimap(hitParticlesMap);

  // construct the combined input container of space point pointers from all
  // configured input sources.
  // pre-compute the total size required so we only need to allocate once
  std::size_t nSpacePoints = 0;
  for (const auto& isp : m_inputSpacePoints) {
    nSpacePoints += (*isp)(ctx).size();
  }

  std::vector<const SimSpacePoint*> spacePointPtrs;
  spacePointPtrs.reserve(nSpacePoints);
  for (const auto& isp : m_inputSpacePoints) {
    for (const auto& spacePoint : (*isp)(ctx)) {
      // since the event store owns the space points, their pointers should be
      // stable and we do not need to create local copies.
      spacePointPtrs.push_back(&spacePoint);
    }
  }

  SimParticleContainer seededParticles;
  SimSeedContainer seeds;
  ProtoTrackContainer tracks;

  seededParticles.reserve(particles.size());
  seeds.reserve(particles.size());
  tracks.reserve(particles.size());

  std::unordered_map<Index, const SimSpacePoint*> spMap;

  for (const auto& spp : spacePointPtrs) {
    if (spp->sourceLinks().empty()) {
      ACTS_WARNING("Missing source link in space point");
      continue;
    }
    for (const auto& slink : spp->sourceLinks()) {
      const IndexSourceLink& islink = slink.get<IndexSourceLink>();
      spMap.emplace(islink.index(), spp);
    }
  }

  for (const auto& particle : particles) {
    // find the corresponding hits for this particle
    const auto& hits =
        makeRange(particleHitsMap.equal_range(particle.particleId()));
    // fill hit indices to create the proto track
    ProtoTrack track;
    track.reserve(hits.size());
    for (const auto& hit : hits) {
      track.push_back(hit.second);
    }

    // The list of hits and the initial start parameters
    if (track.size() < 3) {
      ACTS_WARNING("Particle " << particle << " has less than 3 hits");
      continue;
    }
    // Space points on the proto track
    std::vector<const SimSpacePoint*> spacePointsOnTrack;
    spacePointsOnTrack.reserve(track.size());
    // Loop over the hit index on the proto track to find the space points
    for (const auto& hitIndex : track) {
      auto it = spMap.find(hitIndex);
      if (it != spMap.end()) {
        spacePointsOnTrack.push_back(it->second);
      }
    }
    // At least three space points are required
    if (spacePointsOnTrack.size() < 3) {
      continue;
    }
    // Sort the space points
    std::ranges::sort(spacePointsOnTrack, {}, [](const SimSpacePoint* s) {
      return std::hypot(s->r(), s->z());
    });

    // Loop over the found space points to find the seed with maximum deltaR
    // between the bottom and top space point
    // @todo add the check of deltaZ
    bool seedFound = false;
    std::array<std::size_t, 3> bestSPIndices{};
    double maxDeltaR = std::numeric_limits<double>::min();
    for (std::size_t ib = 0; ib < spacePointsOnTrack.size() - 2; ++ib) {
      for (std::size_t im = ib + 1; im < spacePointsOnTrack.size() - 1; ++im) {
        for (std::size_t it = im + 1; it < spacePointsOnTrack.size(); ++it) {
          double bmDeltaR = std::abs(spacePointsOnTrack[im]->r() -
                                     spacePointsOnTrack[ib]->r());
          double mtDeltaR = std::abs(spacePointsOnTrack[it]->r() -
                                     spacePointsOnTrack[im]->r());
          if (bmDeltaR >= m_cfg.deltaRMin && bmDeltaR <= m_cfg.deltaRMax &&
              mtDeltaR >= m_cfg.deltaRMin && mtDeltaR <= m_cfg.deltaRMax) {
            if ((bmDeltaR + mtDeltaR) > maxDeltaR) {
              maxDeltaR = bmDeltaR + mtDeltaR;
              bestSPIndices = {ib, im, it};
              seedFound = true;
            }
          }
        }
      }
    }

    if (seedFound) {
      SimSeed seed{*spacePointsOnTrack[bestSPIndices[0]],
                   *spacePointsOnTrack[bestSPIndices[1]],
                   *spacePointsOnTrack[bestSPIndices[2]]};
      seed.setVertexZ(
          static_cast<float>(spacePointsOnTrack[bestSPIndices[1]]->z()));

      seededParticles.insert(particle);
      seeds.emplace_back(seed);
      tracks.emplace_back(std::move(track));
    }
  }

  ACTS_VERBOSE("Found " << seeds.size() << " seeds");

  m_outputParticles(ctx, std::move(seededParticles));
  m_outputProtoTracks(ctx, std::move(tracks));
  m_outputSeeds(ctx, std::move(seeds));

  return ActsExamples::ProcessCode::SUCCESS;
}
