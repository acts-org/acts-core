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
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/SympyStepper.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Vertexing/AdaptiveGridDensityVertexFinder.hpp"
#include "Acts/Vertexing/AdaptiveMultiVertexFinder.hpp"
#include "Acts/Vertexing/AdaptiveMultiVertexFitter.hpp"
#include "Acts/Vertexing/GaussianTrackDensity.hpp"
#include "Acts/Vertexing/HelicalTrackLinearizer.hpp"
#include "Acts/Vertexing/IVertexFinder.hpp"
#include "Acts/Vertexing/ImpactPointEstimator.hpp"
#include "Acts/Vertexing/TrackDensityVertexFinder.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "ActsExamples/EventData/ProtoVertex.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimVertex.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Acts {
class MagneticFieldProvider;
}  // namespace Acts

namespace ActsExamples {
struct AlgorithmContext;

class AdaptiveMultiVertexFinderAlgorithm final : public IAlgorithm {
 public:
  using Propagator = Acts::Propagator<Acts::SympyStepper>;
  using Linearizer = Acts::HelicalTrackLinearizer;
  using Fitter = Acts::AdaptiveMultiVertexFitter;
  using Options = Acts::VertexingOptions;

  using VertexCollection = std::vector<Acts::Vertex>;

  enum class SeedFinder { TruthSeeder, GaussianSeeder, AdaptiveGridSeeder };

  struct Config {
    /// Input track parameters collection
    std::string inputTrackParameters;
    /// Optional
    std::string inputTruthParticles;
    /// Optional: Input truth vertex collection. This will only be used if
    /// `seedFinder == SeedFinder::TruthSeeder`.
    std::string inputTruthVertices;
    /// Output proto vertex collection
    std::string outputProtoVertices;
    /// Output vertex collection
    std::string outputVertices = "vertices";

    /// The magnetic field
    std::shared_ptr<Acts::MagneticFieldProvider> bField;

    /// For more information look at `AdaptiveMultiVertexFitter.hpp`
    Acts::AnnealingUtility::Config annealingConfig{9., {1.0}};

    /// For more information look at `AdaptiveMultiVertexFitter.hpp`
    double minWeight = 0.001;
    /// For more information look at `AdaptiveMultiVertexFitter.hpp`
    bool doSmoothing = true;

    /// Maximum number of iterations for the vertex finding
    int maxIterations = 1000;
    /// Use time information in vertex seeder, finder, and fitter
    bool useTime = false;
    /// For more information look at `AdaptiveMultiVertexFinder.hpp`
    double tracksMaxZinterval = 1. * Acts::UnitConstants::mm;
    /// For more information look at `AdaptiveMultiVertexFinder.hpp`
    Acts::Vector4 initialVariances = Acts::Vector4{1e+2, 1e+2, 1e+2, 1e+8};
    /// For more information look at `AdaptiveMultiVertexFinder.hpp`
    bool doFullSplitting = false;
    /// For more information look at `AdaptiveMultiVertexFinder.hpp`
    std::optional<double> tracksMaxSignificance;
    /// For more information look at `AdaptiveMultiVertexFinder.hpp`
    std::optional<double> maxMergeVertexSignificance;

    /// Enum member determining the choice of the vertex seed finder
    SeedFinder seedFinder;
    /// Bin extent in z-direction which is only used with `AdaptiveGridSeeder`
    double spatialBinExtent = 15. * Acts::UnitConstants::um;
    /// Bin extent in t-direction which is only used with `AdaptiveGridSeeder`
    /// and `useTime`
    double temporalBinExtent = 19. * Acts::UnitConstants::mm;
  };

  AdaptiveMultiVertexFinderAlgorithm(const Config& config,
                                     Acts::Logging::Level level);

  /// Find vertices using the adaptive multi vertex finder algorithm.
  ///
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  std::unique_ptr<Acts::IVertexFinder> makeVertexSeeder() const;
  Acts::AdaptiveMultiVertexFinder makeVertexFinder(
      std::shared_ptr<const Acts::IVertexFinder> seedFinder) const;

  Config m_cfg;

  std::shared_ptr<const Acts::BasePropagator> m_propagator;
  Acts::ImpactPointEstimator m_ipEstimator;
  Linearizer m_linearizer;
  std::shared_ptr<Acts::IVertexFinder> m_vertexSeeder;
  Acts::AdaptiveMultiVertexFinder m_vertexFinder;

  ReadDataHandle<TrackParametersContainer> m_inputTrackParameters{
      this, "InputTrackParameters"};
  ReadDataHandle<SimParticleContainer> m_inputTruthParticles{
      this, "InputTruthParticles"};
  ReadDataHandle<SimVertexContainer> m_inputTruthVertices{this,
                                                          "InputTruthVertices"};
  WriteDataHandle<ProtoVertexContainer> m_outputProtoVertices{
      this, "OutputProtoVertices"};
  WriteDataHandle<VertexCollection> m_outputVertices{this, "OutputVertices"};
};

}  // namespace ActsExamples
