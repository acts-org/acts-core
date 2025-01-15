// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <any>
#include <cstdint>
#include <exception>
#include <vector>

#include <c10/cuda/CUDAStream.h>
#include <torch/torch.h>

namespace Acts {

/// Error that is thrown if no edges are found
struct NoEdgesError : std::exception {};

/// Capture the context of the execution
struct ExecutionContext {
  torch::Device device{torch::kCPU};
  std::optional<c10::cuda::CUDAStream> stream;
};

// TODO maybe replace std::any with some kind of variant<unique_ptr<torch>,
// unique_ptr<onnx>>?
// TODO maybe replace input for GraphConstructionBase with some kind of
// boost::multi_array / Eigen::Array

class GraphConstructionBase {
 public:
  /// Perform the graph construction
  ///
  /// @param inputValues Flattened input data
  /// @param numNodes Number of nodes. inputValues.size() / numNodes
  /// then gives the number of features
  /// @param moduleIds Module IDs of the features (used for module-map-like
  /// graph construction)
  /// @param execContext Device & stream information
  /// @return (node_features, edge_features, edge_index)
  virtual std::tuple<std::any, std::any, std::any> operator()(
      std::vector<float> &inputValues, std::size_t numNodes,
      const std::vector<std::uint64_t> &moduleIds,
      const ExecutionContext &execContext = {}) = 0;

  virtual torch::Device device() const = 0;

  virtual ~GraphConstructionBase() = default;
};

class EdgeClassificationBase {
 public:
  /// Perform edge classification
  ///
  /// @param nodeFeatures Node tensor with shape (n_nodes, n_node_features)
  /// @param edgeIndex Edge-index tensor with shape (2, n_edges)
  /// @param edgeFeatures Edge-feature tensor with shape (n_edges, n_edge_features)
  /// @param execContext Device & stream information
  ///
  /// @return (node_features, edge_features, edge_index, edge_scores)
  virtual std::tuple<std::any, std::any, std::any, std::any> operator()(
      std::any nodeFeatures, std::any edgeIndex, std::any edgeFeatures = {},
      const ExecutionContext &execContext = {}) = 0;

  virtual torch::Device device() const = 0;

  virtual ~EdgeClassificationBase() = default;
};

class TrackBuildingBase {
 public:
  /// Perform track building
  ///
  /// @param nodeFeatures Node tensor with shape (n_nodes, n_node_features)
  /// @param edgeIndex Edge-index tensor with shape (2, n_edges)
  /// @param edgeScores Scores of the previous edge classification phase
  /// @param spacepointIDs IDs of the nodes (must have size=n_nodes)
  /// @param execContext Device & stream information
  ///
  /// @return tracks (as vectors of node-IDs)
  virtual std::vector<std::vector<int>> operator()(
      std::any nodeFeatures, std::any edgeIndex, std::any edgeScores,
      std::vector<int> &spacepointIDs,
      const ExecutionContext &execContext = {}) = 0;

  virtual torch::Device device() const = 0;

  virtual ~TrackBuildingBase() = default;
};

}  // namespace Acts
