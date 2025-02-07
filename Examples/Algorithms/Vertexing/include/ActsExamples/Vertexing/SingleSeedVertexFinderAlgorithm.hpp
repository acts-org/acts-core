// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"

#include <string>

namespace ActsExamples {

class SingleSeedVertexFinderAlgorithm final : public IAlgorithm {
 public:
  struct Config {
    /// Optional. Input spacepoints container.
    std::string inputSpacepoints;
    /// Output vertex collection
    std::string outputVertices;
  };

  SingleSeedVertexFinderAlgorithm(const Config& cfg, Acts::Logging::Level lvl);

  /// @brief Find a vertex using spacepoints
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<SimSpacePointContainer> m_inputSpacepoints{this,
                                                            "spacepoints"};
  WriteDataHandle<std::vector<Acts::Vertex>> m_outputVertices{this,
                                                              "fittedVertices"};
};

}  // namespace ActsExamples
