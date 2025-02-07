// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Vertexing/IVertexFinder.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"
#include "ActsExamples/EventData/SimVertex.hpp"

namespace ActsExamples {

class TruthVertexSeeder final : public Acts::IVertexFinder {
 public:
  struct Config {
    bool useXY = false;
    bool useTime = false;
  };

  struct State {
    std::vector<SimVertex> truthVertices;

    std::size_t nextVertexIndex = 0;
  };

  explicit TruthVertexSeeder(const Config& cfg);

  Acts::Result<std::vector<Acts::Vertex>> find(
      const std::vector<Acts::InputTrack>& trackVector,
      const Acts::VertexingOptions& vertexingOptions,
      IVertexFinder::State& state) const final;

  IVertexFinder::State makeState(
      const Acts::MagneticFieldContext& mctx) const final;

  void setTracksToRemove(
      IVertexFinder::State& anyState,
      const std::vector<Acts::InputTrack>& removedTracks) const final;

 private:
  Config m_cfg;
};

}  // namespace ActsExamples
