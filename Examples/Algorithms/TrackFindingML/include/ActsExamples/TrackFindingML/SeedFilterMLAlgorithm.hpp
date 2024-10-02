// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Onnx/SeedClassifier.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/TrackFindingML/AmbiguityResolutionML.hpp"

#include <string>

namespace ActsExamples {

/// Removes seeds that seem to be duplicated and fake.
///
/// The implementation works as follows:
///  1) Cluster together nearby seeds using a DBScan
///  2) For each seed use a neural network to compute a score
///  3) In each cluster keep the seed with the highest score
class SeedFilterMLAlgorithm : public IAlgorithm {
 public:
  struct Config {
    /// Input estimated track parameters collection.
    std::string inputTrackParameters;
    /// Input seeds collection.
    std::string inputSimSeeds;
    /// Path to the ONNX model for the duplicate neural network
    std::string inputSeedFilterNN;
    /// Output estimated track parameters collection.
    std::string outputTrackParameters;
    /// Output seeds collection.
    std::string outputSimSeeds;
    /// Maximum distance between 2 tracks to be clustered in the DBScan
    float epsilonDBScan = 0.03;
    /// Minimum number of tracks to create a cluster in the DBScan
    int minPointsDBScan = 2;
    /// Minimum score a seed need to be selected
    float minSeedScore = 0.1;
    /// Clustering parameters weight for phi used before the DBSCAN
    double clusteringWeighPhi = 1.0;
    /// Clustering parameters weight for eta used before the DBSCAN
    double clusteringWeighEta = 1.0;
    /// Clustering parameters weight for z used before the DBSCAN
    double clusteringWeighZ = 50.0;
    /// Clustering parameters weight for pT used before the DBSCAN
    double clusteringWeighPt = 1.0;
  };

  /// Construct the seed filter algorithm.
  ///
  /// @param cfg is the algorithm configuration
  /// @param lvl is the logging level
  SeedFilterMLAlgorithm(Config cfg, Acts::Logging::Level lvl);

  /// Run the seed filter algorithm.
  ///
  /// @param cxt is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Const access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  // ONNX model for track selection
  Acts::SeedClassifier m_seedClassifier;
  ReadDataHandle<TrackParametersContainer> m_inputTrackParameters{
      this, "InputTrackParameters"};
  ReadDataHandle<SimSeedContainer> m_inputSimSeeds{this, "InputSimSeeds"};
  WriteDataHandle<TrackParametersContainer> m_outputTrackParameters{
      this, "OutputTrackParameters"};
  WriteDataHandle<SimSeedContainer> m_outputSimSeeds{this, "OutputSimSeeds"};
};

}  // namespace ActsExamples
