// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ExaTrkX/BoostTrackBuilding.hpp"

#include "Acts/Utilities/Zip.hpp"

#include <algorithm>
#include <map>

#include <boost/beast/core/span.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <torch/torch.h>

namespace {
template <typename vertex_t, typename weight_t>
auto weaklyConnectedComponents(vertex_t numNodes,
                               boost::beast::span<vertex_t>& rowIndices,
                               boost::beast::span<vertex_t>& colIndices,
                               boost::beast::span<weight_t>& edgeWeights,
                               std::vector<vertex_t>& trackLabels) {
  using Graph =
      boost::adjacency_list<boost::vecS,         // edge list
                            boost::vecS,         // vertex list
                            boost::undirectedS,  // directedness
                            boost::no_property,  // property of vertices
                            weight_t             // property of edges
                            >;

  Graph g(numNodes);

  for (const auto [row, col, weight] :
       Acts::zip(rowIndices, colIndices, edgeWeights)) {
    boost::add_edge(row, col, weight, g);
  }

  return boost::connected_components(g, &trackLabels[0]);
}
}  // namespace

namespace Acts {

std::vector<std::vector<int>> BoostTrackBuilding::operator()(
    std::any /*nodes*/, std::any edges, std::any weights,
    std::vector<int>& spacepointIDs, torch::Device) {
  ACTS_DEBUG("Start track building");
  const auto edgeTensor = std::any_cast<torch::Tensor>(edges).to(torch::kCPU);
  const auto edgeWeightTensor =
      std::any_cast<torch::Tensor>(weights).to(torch::kCPU);

  assert(edgeTensor.size(0) == 2);
  assert(edgeTensor.size(1) == edgeWeightTensor.size(0));

  const auto numSpacepoints = spacepointIDs.size();
  const auto numEdges = static_cast<std::size_t>(edgeWeightTensor.size(0));

  if (numEdges == 0) {
    ACTS_WARNING("No edges remained after edge classification");
    return {};
  }

  using vertex_t = std::int64_t;
  using weight_t = float;

  boost::beast::span<vertex_t> rowIndices(edgeTensor.data_ptr<vertex_t>(),
                                          numEdges);
  boost::beast::span<vertex_t> colIndices(
      edgeTensor.data_ptr<vertex_t>() + numEdges, numEdges);
  boost::beast::span<weight_t> edgeWeights(edgeWeightTensor.data_ptr<float>(),
                                           numEdges);

  std::vector<vertex_t> trackLabels(numSpacepoints);

  auto numberLabels = weaklyConnectedComponents<vertex_t, weight_t>(
      numSpacepoints, rowIndices, colIndices, edgeWeights, trackLabels);

  ACTS_VERBOSE("Number of track labels: " << trackLabels.size());
  ACTS_VERBOSE("Number of unique track labels: " << [&]() {
    std::vector<vertex_t> sorted(trackLabels);
    std::ranges::sort(sorted);
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    return sorted.size();
  }());

  if (trackLabels.size() == 0) {
    return {};
  }

  std::vector<std::vector<int>> trackCandidates(numberLabels);

  for (const auto [label, id] : Acts::zip(trackLabels, spacepointIDs)) {
    trackCandidates[label].push_back(id);
  }

  return trackCandidates;
}

}  // namespace Acts
