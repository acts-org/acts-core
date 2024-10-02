// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/Graph.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"

namespace ActsExamples {

/// Algorithm to create a truth graph for computing truth metrics
/// Requires spacepoints and particles collection
/// Either provide a measurements particles map, or a measurement simhit map +
/// simhits
class TruthGraphBuilder final : public IAlgorithm {
 public:
  struct Config {
    /// Input spacepoint collection
    std::string inputSpacePoints;
    /// Input particles collection
    std::string inputParticles;
    /// Input measurement particles map (Optional).
    std::string inputMeasurementParticlesMap;
    /// Input simhits (Optional).
    std::string inputSimHits;
    /// Input measurement simhit map (Optional).
    std::string inputMeasurementSimHitsMap;
    /// Output truth graph
    std::string outputGraph;

    double targetMinPT = 0.5;
    std::size_t targetMinSize = 3;

    /// Only allow one hit per track & module
    bool uniqueModules = false;
  };

  TruthGraphBuilder(Config cfg, Acts::Logging::Level lvl);

  ~TruthGraphBuilder() override = default;

  ActsExamples::ProcessCode execute(
      const ActsExamples::AlgorithmContext& ctx) const final;

  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  std::vector<std::int64_t> buildFromMeasurements(
      const SimSpacePointContainer& spacepoints,
      const SimParticleContainer& particles,
      const IndexMultimap<ActsFatras::Barcode>& measPartMap) const;

  std::vector<std::int64_t> buildFromSimhits(
      const SimSpacePointContainer& spacepoints,
      const IndexMultimap<Index>& measHitMap, const SimHitContainer& simhits,
      const SimParticleContainer& particles) const;

  ReadDataHandle<SimSpacePointContainer> m_inputSpacePoints{this,
                                                            "InputSpacePoints"};
  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  ReadDataHandle<IndexMultimap<ActsFatras::Barcode>> m_inputMeasParticlesMap{
      this, "InputMeasParticlesMap"};
  ReadDataHandle<SimHitContainer> m_inputSimhits{this, "InputSimhits"};
  ReadDataHandle<IndexMultimap<Index>> m_inputMeasSimhitMap{
      this, "InputMeasSimhitMap"};

  WriteDataHandle<Graph> m_outputGraph{this, "OutputGraph"};
};
}  // namespace ActsExamples
