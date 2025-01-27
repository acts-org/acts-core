// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/ExaTrkX/detail/CudaUtils.cuh"

#include <thrust/execution_policy.h>
#include <thrust/scan.h>

namespace Acts::detail {

template <typename T>
__device__ void swap(T &a, T &b) {
  T tmp = a;
  a = b;
  b = tmp;
}

/// Implementation of the FastSV algorithm as shown in
/// https://arxiv.org/abs/1910.05971
template <typename TEdge, typename TLabel>
__global__ void labelConnectedComponents(std::size_t numEdges,
                                         const TEdge *sourceEdges,
                                         const TEdge *targetEdges,
                                         std::size_t numNodes, TLabel *labels,
                                         TLabel *labelsNext) {
  // Currently this kernel works only with 1 block
  assert(gridDim.x == 1 && gridDim.y == 1 && gridDim.z == 1);

  for (std::size_t i = threadIdx.x; i < numNodes; i += blockDim.x) {
    labels[i] = i;
    labelsNext[i] = i;
  }

  bool changed = false;

  do {
    changed = false;

    //printf("Iteration %i\n", n);

    // Tree hooking for each edge;
    for (std::size_t i = threadIdx.x; i < numEdges; i += blockDim.x) {
      auto u = sourceEdges[i];
      auto v = targetEdges[i];

      if (labels[u] == labels[labels[u]] && labels[v] < labels[u]) {
        labelsNext[labels[u]] = labels[v];
        changed = true;
        //printf("Edge (%i,%i): set labelsNext[%i] = labels[%i] = %i\n", u, v, labels[u], v, labels[v]);
      } else if (labels[v] == labels[labels[v]] && labels[u] < labels[v]) {
        labelsNext[labels[v]] = labels[u];
        changed = true;
        //printf("Edge (%i,%i): set labelsNext[%i] = labels[%i] = %i\n", u, v, labels[v], u, labels[u]);
      } else {
        //printf("Edge (%i,%i): no action\n", u, v);
      }
    }
    __syncthreads();

    for (std::size_t i = threadIdx.x; i < numNodes; i += blockDim.x) {
      labels[i] = labelsNext[i];
    }

    /*if(threadIdx.x == 0 ) {
      for(int i=0; i<numNodes; ++i) {
        printf("Vertex %i - label %i\n", i, labels[i]);
      }
    }*/

    // Shortcutting
    for (std::size_t i = threadIdx.x; i < numNodes; i += blockDim.x) {
      if (labels[i] != labels[labels[i]]) {
        labelsNext[i] = labels[labels[i]];
        //printf("Vertex %i: labelsNext[%i] = labels[%i] = %i\n", i, i, labels[i], labels[labels[i]]);
        changed = true;
      }
    }

    for (std::size_t i = threadIdx.x; i < numNodes; i += blockDim.x) {
      labels[i] = labelsNext[i];
    }

    /*if(threadIdx.x == 0 ) {
      for(int i=0; i<numNodes; ++i) {
        printf("Vertex after Shortcutting %i - label %i\n", i, labels[i]);
      }
    }*/

  } while (__syncthreads_or(changed));
}

template <typename T>
__global__ void makeLabelMask(std::size_t nLabels, const T *labels,
                              T *labelMask) {
  std::size_t i = threadIdx.x + blockDim.x * blockIdx.x;

  if (i >= nLabels) {
    return;
  }

  labelMask[labels[i]] = 1;
}

template <typename T>
__global__ void mapEdgeLabels(std::size_t nLabels, T *labels,
                              const T *mapping) {
  std::size_t i = threadIdx.x + blockDim.x * blockIdx.x;

  if (i >= nLabels) {
    return;
  }

  labels[i] = mapping[labels[i]];
}

template <typename TEdges, typename TLabel>
TLabel connectedComponentsCuda(std::size_t nEdges, const TEdges *sourceEdges,
                               const TEdges *targetEdges, std::size_t nNodes,
                               TLabel *labels, cudaStream_t stream) {
  TLabel *tmpMemory;
  ACTS_CUDA_CHECK(cudaMallocAsync(&tmpMemory, nNodes * sizeof(TLabel), stream));

  // Make synchronization in one block, to avoid that inter-block sync is
  // necessary
  dim3 blockDim = 1024;
  labelConnectedComponents<<<1, blockDim, 1, stream>>>(
      nEdges, sourceEdges, targetEdges, nNodes, labels, tmpMemory);
  ACTS_CUDA_CHECK(cudaGetLastError());

  // Assume we have the following components:
  // 0 3 5 3 0 0

  // Fill a mask which labels survived the connected components algorithm
  // 0 1 2 3 4 5
  // 1 0 0 1 0 1
  ACTS_CUDA_CHECK(
      cudaMemsetAsync(tmpMemory, 0, nNodes * sizeof(TLabel), stream));
  dim3 gridDim = (nNodes + blockDim.x - 1) / blockDim.x;
  makeLabelMask<<<gridDim, blockDim, 0, stream>>>(nNodes, labels, tmpMemory);
  ACTS_CUDA_CHECK(cudaGetLastError());

  // Exclusive prefix sum on the label mask
  // 0 1 2 3 4 5
  // 0 1 1 1 2 2
  thrust::exclusive_scan(thrust::device.on(stream), tmpMemory,
                         tmpMemory + nNodes, tmpMemory);

  // Remap edge labels with values in prefix sum
  // 0 -> 0, 3 -> 1, 5 -> 2
  mapEdgeLabels<<<gridDim, blockDim, 0, stream>>>(nNodes, labels, tmpMemory);
  ACTS_CUDA_CHECK(cudaGetLastError());

  TLabel nLabels;
  ACTS_CUDA_CHECK(cudaMemcpyAsync(&nLabels, &tmpMemory[nNodes - 1],
                                  sizeof(TLabel), cudaMemcpyDeviceToHost,
                                  stream));

  ACTS_CUDA_CHECK(cudaFreeAsync(tmpMemory, stream));
  ACTS_CUDA_CHECK(cudaStreamSynchronize(stream));

  return nLabels;
}

}  // namespace Acts::detail
