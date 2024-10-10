// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Vertexing/SingleSeedVertexFinderAlgorithm.hpp"

#include "Acts/Vertexing/SingleSeedVertexFinder.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <chrono>
#include <vector>

ActsExamples::SingleSeedVertexFinderAlgorithm::SingleSeedVertexFinderAlgorithm(
    const Config& cfg, Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("SingleSeedVertexFinder", lvl), m_cfg(cfg) {
  if (m_cfg.inputSpacepoints.empty()) {
    ACTS_ERROR("You have to provide seeds");
  }
  if (m_cfg.outputVertices.empty()) {
    ACTS_ERROR("Missing output vertices collection");
  }

  m_inputSpacepoints.initialize(m_cfg.inputSpacepoints);
  m_outputVertices.initialize(m_cfg.outputVertices);
}

ActsExamples::ProcessCode
ActsExamples::SingleSeedVertexFinderAlgorithm::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  // retrieve input seeds
  const std::vector<ActsExamples::SimSpacePoint>& inputSpacepoints =
      m_inputSpacepoints(ctx);

  Acts::SingleSeedVertexFinder<ActsExamples::SimSpacePoint>::Config
      singleSeedVtxCfg;
  Acts::SingleSeedVertexFinder<ActsExamples::SimSpacePoint>
      SingleSeedVertexFinder(singleSeedVtxCfg);

  // find vertices and measure elapsed time
  auto t1 = std::chrono::high_resolution_clock::now();
  auto vtx = SingleSeedVertexFinder.findVertex(inputSpacepoints);
  auto t2 = std::chrono::high_resolution_clock::now();
  if (vtx.ok()) {
    ACTS_INFO("Found a vertex in the event in " << (t2 - t1).count() / 1e6
                                                << " ms");
    ACTS_INFO("Found vertex at x = " << vtx.value()[0]
                                     << "mm, y = " << vtx.value()[1]
                                     << "mm, z = " << vtx.value()[2] << "mm");

    std::vector<Acts::Vertex> vertexCollection;
    vertexCollection.emplace_back(vtx.value());

    // store found vertices
    m_outputVertices(ctx, std::move(vertexCollection));
  } else {
    ACTS_INFO("Not found a vertex in the event after "
              << (t2 - t1).count() / 1e6 << " ms");
  }

  return ActsExamples::ProcessCode::SUCCESS;
}
