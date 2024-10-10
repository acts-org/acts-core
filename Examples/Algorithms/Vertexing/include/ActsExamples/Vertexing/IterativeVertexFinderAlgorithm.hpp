// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/EventData/Charge.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/SympyStepper.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Vertexing/FullBilloirVertexFitter.hpp"
#include "Acts/Vertexing/HelicalTrackLinearizer.hpp"
#include "Acts/Vertexing/ImpactPointEstimator.hpp"
#include "Acts/Vertexing/IterativeVertexFinder.hpp"
#include "Acts/Vertexing/TrackDensityVertexFinder.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"
#include "ActsExamples/EventData/ProtoVertex.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Acts {
class MagneticFieldProvider;
}  // namespace Acts

namespace ActsExamples {
struct AlgorithmContext;

class IterativeVertexFinderAlgorithm final : public IAlgorithm {
 public:
  using Propagator = Acts::Propagator<Acts::SympyStepper>;
  using Linearizer = Acts::HelicalTrackLinearizer;
  using Fitter = Acts::FullBilloirVertexFitter;
  using Seeder = Acts::TrackDensityVertexFinder;
  using Finder = Acts::IterativeVertexFinder;
  using Options = Acts::VertexingOptions;

  using VertexCollection = std::vector<Acts::Vertex>;

  struct Config {
    /// Optional. Input track parameters collection
    std::string inputTrackParameters;
    /// Output proto vertex collection
    std::string outputProtoVertices;
    /// Output vertex collection
    std::string outputVertices = "vertices";

    /// The magnetic field
    std::shared_ptr<Acts::MagneticFieldProvider> bField;

    /// Maximum number of iterations for the vertex finding
    int maxIterations = 1000;
  };

  IterativeVertexFinderAlgorithm(const Config& config,
                                 Acts::Logging::Level level);

  /// Find vertices using iterative vertex finder algorithm.
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<TrackParametersContainer> m_inputTrackParameters{
      this, "InputTrackParameters"};

  WriteDataHandle<ProtoVertexContainer> m_outputProtoVertices{
      this, "OutputProtoVertices"};

  WriteDataHandle<VertexCollection> m_outputVertices{this, "OutputVertices"};
};

}  // namespace ActsExamples
