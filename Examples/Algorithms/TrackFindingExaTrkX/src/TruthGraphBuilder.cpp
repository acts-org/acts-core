// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <Acts/Definitions/Units.hpp>
#include <ActsExamples/TrackFindingExaTrkX/TruthGraphBuilder.hpp>

#include <algorithm>

using namespace Acts::UnitLiterals;

namespace ActsExamples {

TruthGraphBuilder::TruthGraphBuilder(Config config, Acts::Logging::Level level)
    : ActsExamples::IAlgorithm("TruthGraphBuilder", level),
      m_cfg(std::move(config)) {
  m_inputSpacePoints.initialize(m_cfg.inputSpacePoints);
  m_inputParticles.initialize(m_cfg.inputParticles);
  m_outputGraph.initialize(m_cfg.outputGraph);

  m_inputMeasParticlesMap.maybeInitialize(m_cfg.inputMeasurementParticlesMap);
  m_inputSimhits.maybeInitialize(m_cfg.inputSimHits);
  m_inputMeasSimhitMap.maybeInitialize(m_cfg.inputMeasurementSimHitsMap);

  bool a = m_inputMeasParticlesMap.isInitialized();
  bool b =
      m_inputSimhits.isInitialized() && m_inputMeasSimhitMap.isInitialized();

  // Logical XOR operation
  if (!a != !b) {
    throw std::invalid_argument("Missing inputs, cannot build truth graph");
  }
}

std::vector<std::int64_t> TruthGraphBuilder::buildFromMeasurements(
    const SimSpacePointContainer& spacepoints,
    const SimParticleContainer& particles,
    const IndexMultimap<ActsFatras::Barcode>& measPartMap) const {
  if (m_cfg.targetMinPT < 500_MeV) {
    ACTS_WARNING(
        "truth graph building based on distance from origin, this breaks down "
        "for low pT particles. Consider using a higher target pT value");
  }

  // Associate tracks to graph, collect momentum
  std::unordered_map<ActsFatras::Barcode, std::vector<std::size_t>> tracks;

  for (auto i = 0ul; i < spacepoints.size(); ++i) {
    const auto measId =
        spacepoints[i].sourceLinks()[0].template get<IndexSourceLink>().index();

    auto [a, b] = measPartMap.equal_range(measId);
    for (auto it = a; it != b; ++it) {
      tracks[it->second].push_back(i);
    }
  }

  // Collect edges for truth graph and target graph
  std::vector<std::int64_t> graph;
  std::size_t notFoundParticles = 0;
  std::size_t moduleDuplicatesRemoved = 0;

  for (auto& [pid, track] : tracks) {
    auto found = particles.find(pid);
    if (found == particles.end()) {
      ACTS_VERBOSE("Did not find " << pid << ", skip track");
      notFoundParticles++;
      continue;
    }

    if (found->transverseMomentum() < m_cfg.targetMinPT ||
        track.size() < m_cfg.targetMinSize) {
      continue;
    }

    const Acts::Vector3 vtx = found->fourPosition().segment<3>(0);
    auto radiusForOrdering = [&](std::size_t i) {
      const auto& sp = spacepoints[i];
      return std::hypot(sp.x() - vtx[0], sp.y() - vtx[1], sp.z() - vtx[2]);
    };

    // Sort by radius (this breaks down if the particle has to low momentum)
    std::ranges::sort(track, {},
                      [&](const auto& t) { return radiusForOrdering(t); });

    if (m_cfg.uniqueModules) {
      auto newEnd = std::unique(
          track.begin(), track.end(), [&](const auto& a, const auto& b) {
            auto gidA = spacepoints[a]
                            .sourceLinks()[0]
                            .template get<IndexSourceLink>()
                            .geometryId();
            auto gidB = spacepoints[b]
                            .sourceLinks()[0]
                            .template get<IndexSourceLink>()
                            .geometryId();
            return gidA == gidB;
          });
      moduleDuplicatesRemoved += std::distance(newEnd, track.end());
      track.erase(newEnd, track.end());
    }

    for (auto i = 0ul; i < track.size() - 1; ++i) {
      graph.push_back(track[i]);
      graph.push_back(track[i + 1]);
    }
  }

  ACTS_DEBUG("Did not find particles for " << notFoundParticles << " tracks");
  if (moduleDuplicatesRemoved > 0) {
    ACTS_DEBUG(
        "Removed " << moduleDuplicatesRemoved
                   << " hit to ensure a unique hit per track and module");
  }

  return graph;
}

struct HitInfo {
  std::size_t spacePointIndex;
  std::int32_t hitIndex;
};

std::vector<std::int64_t> TruthGraphBuilder::buildFromSimhits(
    const SimSpacePointContainer& spacepoints,
    const IndexMultimap<Index>& measHitMap, const SimHitContainer& simhits,
    const SimParticleContainer& particles) const {
  // Associate tracks to graph, collect momentum
  std::unordered_map<ActsFatras::Barcode, std::vector<HitInfo>> tracks;

  for (auto i = 0ul; i < spacepoints.size(); ++i) {
    const auto measId =
        spacepoints[i].sourceLinks()[0].template get<IndexSourceLink>().index();

    auto [a, b] = measHitMap.equal_range(measId);
    for (auto it = a; it != b; ++it) {
      const auto& hit = *simhits.nth(it->second);

      tracks[hit.particleId()].push_back({i, hit.index()});
    }
  }

  // Collect edges for truth graph and target graph
  std::vector<std::int64_t> truthGraph;

  for (auto& [pid, track] : tracks) {
    // Sort by hit index, so the edges are connected correctly
    std::ranges::sort(track, {}, [](const auto& t) { return t.hitIndex; });

    auto found = particles.find(pid);
    if (found == particles.end()) {
      ACTS_WARNING("Did not find " << pid << ", skip track");
      continue;
    }

    for (auto i = 0ul; i < track.size() - 1; ++i) {
      if (found->transverseMomentum() > m_cfg.targetMinPT &&
          track.size() >= m_cfg.targetMinSize) {
        truthGraph.push_back(track[i].spacePointIndex);
        truthGraph.push_back(track[i + 1].spacePointIndex);
      }
    }
  }

  return truthGraph;
}

ProcessCode TruthGraphBuilder::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  // Read input data
  const auto& spacepoints = m_inputSpacePoints(ctx);
  const auto& particles = m_inputParticles(ctx);

  auto edges = (m_inputMeasParticlesMap.isInitialized())
                   ? buildFromMeasurements(spacepoints, particles,
                                           m_inputMeasParticlesMap(ctx))
                   : buildFromSimhits(spacepoints, m_inputMeasSimhitMap(ctx),
                                      m_inputSimhits(ctx), particles);

  ACTS_DEBUG("Truth track edges: " << edges.size() / 2);

  Graph g;
  g.edges = std::move(edges);

  m_outputGraph(ctx, std::move(g));

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
