// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <Acts/Utilities/Logger.hpp>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include <boost/range/combine.hpp>
#include <cugraph/algorithms.hpp>
#include <cugraph/graph.hpp>
#include <cugraph/graph_functions.hpp>
#include <cugraph/graph_view.hpp>
#include <cugraph/partition_manager.hpp>
#include <cugraph/utilities/error.hpp>
#include <raft/cudart_utils.h>
#include <raft/handle.hpp>

#ifndef CUDA_RT_CALL
#define CUDA_RT_CALL(call)                                                    \
  {                                                                           \
    cudaError_t cudaStatus = call;                                            \
    if (cudaSuccess != cudaStatus) {                                          \
      fprintf(stderr,                                                         \
              "ERROR: CUDA RT call \"%s\" in line %d of file %s failed with " \
              "%s (%d).\n",                                                   \
              #call, __LINE__, __FILE__, cudaGetErrorString(cudaStatus),      \
              cudaStatus);                                                    \
    }                                                                         \
  }
#endif  // CUDA_RT_CALL

template <typename vertex_t, typename edge_t, typename weight_t>
__global__ void weaklyConnectedComponents(std::vector<vertex_t>& rowIndices,
                                          std::vector<vertex_t>& colIndices,
                                          std::vector<weight_t>& edgeWeights,
                                          std::vector<vertex_t>& trackLabels,
                                          const Acts::Logger& logger) {
  cudaStream_t stream;
  CUDA_RT_CALL(cudaStreamCreate(&stream));

  ACTS_VERBOSE("Weakly components Start");
  ACTS_VERBOSE("edge size: " << rowIndices.size() << " " << colIndices.size());
  raft::handle_t handle{stream};

  cugraph::graph_t<vertex_t, edge_t, weight_t, false, false> graph(handle);

  // learn from matrix_market_file_utilities.cu
  vertex_t maxVertexID_row =
      *std::max_element(rowIndices.begin(), rowIndices.end());
  vertex_t maxVertexID_col =
      *std::max_element(colIndices.begin(), colIndices.end());
  vertex_t maxVertex = std::max(maxVertexID_row, maxVertexID_col);

  vertex_t number_of_vertices = maxVertex;
  rmm::device_uvector<vertex_t> d_vertices(number_of_vertices,
                                           handle.get_stream());
  std::vector<vertex_t> vertex_idx(number_of_vertices);
  for (vertex_t idx = 0; idx < number_of_vertices; idx++) {
    vertex_idx[idx] = idx;
  }

  rmm::device_uvector<vertex_t> src_v(rowIndices.size(), handle.get_stream());
  rmm::device_uvector<vertex_t> dst_v(colIndices.size(), handle.get_stream());
  rmm::device_uvector<weight_t> weights_v(edgeWeights.size(),
                                          handle.get_stream());

  raft::update_device(src_v.data(), rowIndices.data(), rowIndices.size(),
                      handle.get_stream());
  raft::update_device(dst_v.data(), colIndices.data(), colIndices.size(),
                      handle.get_stream());
  raft::update_device(weights_v.data(), edgeWeights.data(), edgeWeights.size(),
                      handle.get_stream());
  raft::update_device(d_vertices.data(), vertex_idx.data(), vertex_idx.size(),
                      handle.get_stream());

  std::tie(graph, std::ignore) =
      cugraph::create_graph_from_edgelist<vertex_t, edge_t, weight_t, false,
                                          false>(
          handle, std::move(d_vertices), std::move(src_v), std::move(dst_v),
          std::move(weights_v), cugraph::graph_properties_t{true, false},
          false);

  auto graph_view = graph.view();
  CUDA_TRY(cudaDeviceSynchronize());  // for consistent performance measurement

  rmm::device_uvector<vertex_t> d_components(
      graph_view.get_number_of_vertices(), handle.get_stream());

  ACTS_VERBOSE("2back from construct_graph");
  cugraph::weakly_connected_components(handle, graph_view, d_components.data());

  ACTS_VERBOSE("number of components: " << d_components.size());
  raft::update_host(trackLabels.data(), d_components.data(),
                    d_components.size(), handle.get_stream());
}
