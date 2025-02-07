// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Plugins/ExaTrkX/ExaTrkXPipeline.hpp"
#include "Acts/Plugins/ExaTrkX/Stages.hpp"
#include "Acts/Plugins/ExaTrkX/TorchGraphStoreHook.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsExamples/EventData/Graph.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/TrackFindingExaTrkX/TruthGraphBuilder.hpp"

#include <mutex>
#include <string>
#include <vector>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

namespace ActsExamples {

class TrackFindingAlgorithmExaTrkX final : public IAlgorithm {
 public:
  enum class NodeFeature {
    // SP features
    eR,
    ePhi,
    eX,
    eY,
    eZ,
    eEta,
    // Single cluster features
    eCellCount,
    eChargeSum,
    eClusterLoc0,
    eClusterLoc1,
    // Cluster 1 features
    eCluster1X,
    eCluster1Y,
    eCluster1Z,
    eCluster1R,
    eCluster1Phi,
    eCluster1Eta,
    eCellCount1,
    eChargeSum1,
    eLocEta1,
    eLocPhi1,
    eLocDir01,
    eLocDir11,
    eLocDir21,
    eLengthDir01,
    eLengthDir11,
    eLengthDir21,
    eGlobEta1,
    eGlobPhi1,
    eEtaAngle1,
    ePhiAngle1,
    // Cluster 2 features
    eCluster2X,
    eCluster2Y,
    eCluster2Z,
    eCluster2R,
    eCluster2Phi,
    eCluster2Eta,
    eCellCount2,
    eChargeSum2,
    eLocEta2,
    eLocPhi2,
    eLocDir02,
    eLocDir12,
    eLocDir22,
    eLengthDir02,
    eLengthDir12,
    eLengthDir22,
    eGlobEta2,
    eGlobPhi2,
    eEtaAngle2,
    ePhiAngle2,
  };

  struct Config {
    /// Input spacepoints collection.
    std::string inputSpacePoints;
    /// Input cluster information (Optional).
    std::string inputClusters;
    /// Input truth graph (Optional).
    std::string inputTruthGraph;
    /// Output prototracks
    std::string outputProtoTracks;
    /// Output graph (optional)
    std::string outputGraph;

    /// Graph constructor
    std::shared_ptr<Acts::GraphConstructionBase> graphConstructor;

    /// List of edge classifiers
    std::vector<std::shared_ptr<Acts::EdgeClassificationBase>> edgeClassifiers;

    /// The track builder
    std::shared_ptr<Acts::TrackBuildingBase> trackBuilder;

    /// Node features
    std::vector<NodeFeature> nodeFeatures = {NodeFeature::eR, NodeFeature::ePhi,
                                             NodeFeature::eZ};

    /// Feature scales
    std::vector<float> featureScales = {1.f, 1.f, 1.f};

    /// Remove track candidates with 2 or less hits
    std::size_t minMeasurementsPerTrack = 3;

    /// Optionally remap the geometry Ids that are put into the chain
    std::shared_ptr<GeometryIdMapActsAthena> geometryIdMap;
  };

  /// Constructor of the track finding algorithm
  ///
  /// @param cfg is the config struct to configure the algorithm
  /// @param level is the logging level
  TrackFindingAlgorithmExaTrkX(Config cfg, Acts::Logging::Level lvl);

  ~TrackFindingAlgorithmExaTrkX() override = default;

  /// Framework execute method of the track finding algorithm
  ///
  /// @param ctx is the algorithm context that holds event-wise information
  /// @return a process code to steer the algorithm flow
  ActsExamples::ProcessCode execute(
      const ActsExamples::AlgorithmContext& ctx) const final;

  /// Finalize and print timing
  ActsExamples::ProcessCode finalize() final;

  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  Acts::ExaTrkXPipeline m_pipeline;
  mutable std::mutex m_mutex;

  using Accumulator = boost::accumulators::accumulator_set<
      float,
      boost::accumulators::features<
          boost::accumulators::tag::mean, boost::accumulators::tag::variance,
          boost::accumulators::tag::max, boost::accumulators::tag::min>>;

  mutable struct {
    Accumulator preprocessingTime;
    Accumulator graphBuildingTime;
    std::vector<Accumulator> classifierTimes;
    Accumulator trackBuildingTime;
    Accumulator postprocessingTime;
  } m_timing;

  ReadDataHandle<SimSpacePointContainer> m_inputSpacePoints{this,
                                                            "InputSpacePoints"};
  ReadDataHandle<ClusterContainer> m_inputClusters{this, "InputClusters"};

  ReadDataHandle<Graph> m_inputTruthGraph{this, "InputTruthGraph"};
  WriteDataHandle<ProtoTrackContainer> m_outputProtoTracks{this,
                                                           "OutputProtoTracks"};
  WriteDataHandle<Graph> m_outputGraph{this, "OutputGraph"};
};

}  // namespace ActsExamples
