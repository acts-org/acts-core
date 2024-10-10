// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TrackFindingML/SeedFilterMLAlgorithm.hpp"

#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/TrackFindingML/SeedFilterDBScanClustering.hpp"

#include <iterator>
#include <map>

ActsExamples::SeedFilterMLAlgorithm::SeedFilterMLAlgorithm(
    ActsExamples::SeedFilterMLAlgorithm::Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("SeedFilterMLAlgorithm", lvl),
      m_cfg(std::move(cfg)),
      m_seedClassifier(m_cfg.inputSeedFilterNN.c_str()) {
  if (m_cfg.inputTrackParameters.empty()) {
    throw std::invalid_argument("Missing track parameters input collection");
  }
  if (m_cfg.inputSimSeeds.empty()) {
    throw std::invalid_argument("Missing seed input collection");
  }
  if (m_cfg.outputTrackParameters.empty()) {
    throw std::invalid_argument("Missing track parameters output collection");
  }
  if (m_cfg.outputSimSeeds.empty()) {
    throw std::invalid_argument("Missing seed output collection");
  }
  m_inputTrackParameters.initialize(m_cfg.inputTrackParameters);
  m_inputSimSeeds.initialize(m_cfg.inputSimSeeds);
  m_outputTrackParameters.initialize(m_cfg.outputTrackParameters);
  m_outputSimSeeds.initialize(m_cfg.outputSimSeeds);
}

ActsExamples::ProcessCode ActsExamples::SeedFilterMLAlgorithm::execute(
    const AlgorithmContext& ctx) const {
  // Read input data
  const auto& seeds = m_inputSimSeeds(ctx);
  const auto& params = m_inputTrackParameters(ctx);
  if (seeds.size() != params.size()) {
    throw std::invalid_argument(
        "The number of seeds and track parameters is different");
  }

  Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      networkInput(seeds.size(), 14);
  std::vector<std::array<double, 4>> clusteringParams;
  // Loop over the seed and parameters to fill the input for the clustering
  // and the NN
  for (std::size_t i = 0; i < seeds.size(); i++) {
    // Compute the track parameters
    double pT = std::abs(1.0 / params[i].parameters()[Acts::eBoundQOverP]) *
                std::sin(params[i].parameters()[Acts::eBoundTheta]);
    double eta =
        std::atanh(std::cos(params[i].parameters()[Acts::eBoundTheta]));
    double phi = params[i].parameters()[Acts::eBoundPhi];

    // Fill and weight the clustering inputs
    clusteringParams.push_back(
        {phi / m_cfg.clusteringWeighPhi, eta / m_cfg.clusteringWeighEta,
         seeds[i].z() / m_cfg.clusteringWeighZ, pT / m_cfg.clusteringWeighPt});

    // Fill the NN input
    networkInput.row(i) << pT, eta, phi, seeds[i].sp()[0]->x(),
        seeds[i].sp()[0]->y(), seeds[i].sp()[0]->z(), seeds[i].sp()[1]->x(),
        seeds[i].sp()[1]->y(), seeds[i].sp()[1]->z(), seeds[i].sp()[2]->x(),
        seeds[i].sp()[2]->y(), seeds[i].sp()[2]->z(), seeds[i].z(),
        seeds[i].seedQuality();
  }

  // Cluster the tracks using DBscan
  auto cluster = Acts::dbscanSeedClustering(
      clusteringParams, m_cfg.epsilonDBScan, m_cfg.minPointsDBScan);

  // Select the ID of the track we want to keep
  std::vector<std::size_t> goodSeed = m_seedClassifier.solveAmbiguity(
      cluster, networkInput, m_cfg.minSeedScore);

  // Create the output seed collection
  SimSeedContainer outputSeeds;
  outputSeeds.reserve(goodSeed.size());

  // Create the output track parameters collection
  TrackParametersContainer outputTrackParameters;
  outputTrackParameters.reserve(goodSeed.size());

  for (auto i : goodSeed) {
    outputSeeds.push_back(seeds[i]);
    outputTrackParameters.push_back(params[i]);
  }

  m_outputSimSeeds(ctx, SimSeedContainer{outputSeeds});
  m_outputTrackParameters(ctx, TrackParametersContainer{outputTrackParameters});

  return ActsExamples::ProcessCode::SUCCESS;
}
