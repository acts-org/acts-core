// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Onnx/OnnxRuntimeBase.hpp"

#include <vector>

#include <onnxruntime_cxx_api.h>

namespace Acts {

/// Onnx model implementation for seed scoring and selection
class SeedClassifier {
 public:
  /// Construct the scoring algorithm.
  ///
  /// @param modelPath path to the model file
  SeedClassifier(const char* modelPath)
      : m_env(ORT_LOGGING_LEVEL_WARNING, "MLSeedClassifier"),
        m_duplicateClassifier(m_env, modelPath) {};

  /// Compute a score for each seed to be used in the seed selection
  ///
  /// @param networkInput input of the network
  /// @return a vector of vector of seed score. Due to the architecture of the network each seed only have a size 1 score vector.
  std::vector<std::vector<float>> inferScores(
      Acts::NetworkBatchInput& networkInput) const {
    // Use the network to compute a score for all the Seeds.
    std::vector<std::vector<float>> outputTensor =
        m_duplicateClassifier.runONNXInference(networkInput);
    return outputTensor;
  }

  /// Select the seed associated with each cluster based on the score vector
  ///
  /// @param clusters is a vector of clusters, each cluster corresponds to a vector of seedIDs
  /// @param outputTensor is the score vector obtained from inferScores.
  /// @param minSeedScore is the minimum score a seed needs to be selected
  /// @return a vector of seedIDs corresponding tho the good seeds
  std::vector<std::size_t> seedSelection(
      std::vector<std::vector<std::size_t>>& clusters,
      std::vector<std::vector<float>>& outputTensor,
      float minSeedScore = 0.1) const {
    std::vector<std::size_t> goodSeeds;
    // Loop over all the cluster and only keep the seed with the highest score
    // in each cluster
    for (const auto& cluster : clusters) {
      std::size_t bestseedID = 0;
      float bestSeedScore = 0;
      for (const auto& seed : cluster) {
        if (outputTensor[seed][0] > bestSeedScore) {
          bestSeedScore = outputTensor[seed][0];
          bestseedID = seed;
        }
      }
      if (bestSeedScore >= minSeedScore) {
        goodSeeds.push_back(bestseedID);
      }
    }
    return goodSeeds;
  }

  /// Select the seed associated with each cluster
  ///
  /// @param clusters is a map of clusters, each cluster correspond to a vector of seed ID
  /// @param networkInput input of the network
  /// @param minSeedScore is the minimum score a seed need to be selected
  /// @return a vector of seedID corresponding the the good seeds
  std::vector<std::size_t> solveAmbiguity(
      std::vector<std::vector<std::size_t>>& clusters,
      Acts::NetworkBatchInput& networkInput, float minSeedScore = 0.1) const {
    std::vector<std::vector<float>> outputTensor = inferScores(networkInput);
    std::vector<std::size_t> goodSeeds =
        seedSelection(clusters, outputTensor, minSeedScore);
    return goodSeeds;
  }

 private:
  // ONNX environment
  Ort::Env m_env;
  // ONNX model for the duplicate neural network
  Acts::OnnxRuntimeBase m_duplicateClassifier;
};

}  // namespace Acts
