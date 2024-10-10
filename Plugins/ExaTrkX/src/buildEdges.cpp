// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ExaTrkX/detail/buildEdges.hpp"

#include "Acts/Plugins/ExaTrkX/detail/TensorVectorConversion.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/KDTree.hpp"

#include <iostream>
#include <mutex>
#include <vector>

#include <torch/script.h>
#include <torch/torch.h>

#ifndef ACTS_EXATRKX_CPUONLY
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <grid/counting_sort.h>
#include <grid/find_nbrs.h>
#include <grid/grid.h>
#include <grid/insert_points.h>
#include <grid/prefix_sum.h>
#endif

using namespace torch::indexing;

torch::Tensor Acts::detail::postprocessEdgeTensor(torch::Tensor edges,
                                                  bool removeSelfLoops,
                                                  bool removeDuplicates,
                                                  bool flipDirections) {
  // Remove self-loops
  if (removeSelfLoops) {
    torch::Tensor selfLoopMask = edges.index({0}) != edges.index({1});
    edges = edges.index({Slice(), selfLoopMask});
  }

  // Remove duplicates
  if (removeDuplicates) {
    torch::Tensor mask = edges.index({0}) > edges.index({1});
    edges.index_put_({Slice(), mask}, edges.index({Slice(), mask}).flip(0));
    edges = std::get<0>(torch::unique_dim(edges, -1, false));
  }

  // Randomly flip direction
  if (flipDirections) {
    torch::Tensor random_cut_keep = torch::randint(2, {edges.size(1)});
    torch::Tensor random_cut_flip = 1 - random_cut_keep;
    torch::Tensor keep_edges =
        edges.index({Slice(), random_cut_keep.to(torch::kBool)});
    torch::Tensor flip_edges =
        edges.index({Slice(), random_cut_flip.to(torch::kBool)}).flip({0});
    edges = torch::cat({keep_edges, flip_edges}, 1);
  }

  return edges.toType(torch::kInt64);
}

torch::Tensor Acts::detail::buildEdgesFRNN(torch::Tensor &embedFeatures,
                                           float rVal, int kVal,
                                           bool flipDirections) {
#ifndef ACTS_EXATRKX_CPUONLY
  const auto device = embedFeatures.device();

  const std::int64_t numSpacepoints = embedFeatures.size(0);
  const int dim = embedFeatures.size(1);

  const int grid_params_size = 8;
  const int grid_delta_idx = 3;
  const int grid_total_idx = 7;
  const int grid_max_res = 128;
  const int grid_dim = 3;

  if (dim < 3) {
    throw std::runtime_error("DIM < 3 is not supported for now.\n");
  }

  const float radius_cell_ratio = 2.0;
  const int batch_size = 1;
  int G = -1;

  // Set up grid properties
  torch::Tensor grid_min;
  torch::Tensor grid_max;
  torch::Tensor grid_size;

  torch::Tensor embedTensor = embedFeatures.reshape({1, numSpacepoints, dim});
  torch::Tensor gridParamsCuda =
      torch::zeros({batch_size, grid_params_size}, device).to(torch::kFloat32);
  torch::Tensor r_tensor = torch::full({batch_size}, rVal, device);
  torch::Tensor lengths = torch::full({batch_size}, numSpacepoints, device);

  // build the grid
  for (int i = 0; i < batch_size; i++) {
    torch::Tensor allPoints =
        embedTensor.index({i, Slice(None, lengths.index({i}).item().to<long>()),
                           Slice(None, grid_dim)});
    grid_min = std::get<0>(allPoints.min(0));
    grid_max = std::get<0>(allPoints.max(0));
    gridParamsCuda.index_put_({i, Slice(None, grid_delta_idx)}, grid_min);

    grid_size = grid_max - grid_min;

    float cell_size =
        r_tensor.index({i}).item().to<float>() / radius_cell_ratio;

    if (cell_size < (grid_size.min().item().to<float>() / grid_max_res)) {
      cell_size = grid_size.min().item().to<float>() / grid_max_res;
    }

    gridParamsCuda.index_put_({i, grid_delta_idx}, 1 / cell_size);

    gridParamsCuda.index_put_({i, Slice(1 + grid_delta_idx, grid_total_idx)},
                              floor(grid_size / cell_size) + 1);

    gridParamsCuda.index_put_(
        {i, grid_total_idx},
        gridParamsCuda.index({i, Slice(1 + grid_delta_idx, grid_total_idx)})
            .prod());

    if (G < gridParamsCuda.index({i, grid_total_idx}).item().to<int>()) {
      G = gridParamsCuda.index({i, grid_total_idx}).item().to<int>();
    }
  }

  torch::Tensor pc_grid_cnt =
      torch::zeros({batch_size, G}, device).to(torch::kInt32);
  torch::Tensor pc_grid_cell =
      torch::full({batch_size, numSpacepoints}, -1, device).to(torch::kInt32);
  torch::Tensor pc_grid_idx =
      torch::full({batch_size, numSpacepoints}, -1, device).to(torch::kInt32);

  // put spacepoints into the grid
  InsertPointsCUDA(embedTensor, lengths.to(torch::kInt64), gridParamsCuda,
                   pc_grid_cnt, pc_grid_cell, pc_grid_idx, G);

  torch::Tensor pc_grid_off =
      torch::full({batch_size, G}, 0, device).to(torch::kInt32);
  torch::Tensor grid_params = gridParamsCuda.to(torch::kCPU);

  // for loop seems not to be necessary anymore
  pc_grid_off = PrefixSumCUDA(pc_grid_cnt, grid_params);

  torch::Tensor sorted_points =
      torch::zeros({batch_size, numSpacepoints, dim}, device)
          .to(torch::kFloat32);
  torch::Tensor sorted_points_idxs =
      torch::full({batch_size, numSpacepoints}, -1, device).to(torch::kInt32);

  CountingSortCUDA(embedTensor, lengths.to(torch::kInt64), pc_grid_cell,
                   pc_grid_idx, pc_grid_off, sorted_points, sorted_points_idxs);

  auto [indices, distances] = FindNbrsCUDA(
      sorted_points, sorted_points, lengths.to(torch::kInt64),
      lengths.to(torch::kInt64), pc_grid_off.to(torch::kInt32),
      sorted_points_idxs, sorted_points_idxs,
      gridParamsCuda.to(torch::kFloat32), kVal, r_tensor, r_tensor * r_tensor);
  torch::Tensor positiveIndices = indices >= 0;

  torch::Tensor repeatRange = torch::arange(positiveIndices.size(1), device)
                                  .repeat({1, positiveIndices.size(2), 1})
                                  .transpose(1, 2);

  torch::Tensor stackedEdges = torch::stack(
      {repeatRange.index({positiveIndices}), indices.index({positiveIndices})});

  return postprocessEdgeTensor(std::move(stackedEdges), true, true,
                               flipDirections);
#else
  throw std::runtime_error(
      "ACTS not compiled with CUDA, cannot run Acts::buildEdgesFRNN");
#endif
}

/// This is a very unsophisticated span implementation to avoid data copies in
/// the KDTree search.
/// Should be replaced with std::span when possible
template <typename T, std::size_t S>
struct Span {
  T *ptr;

  auto size() const { return S; }

  using const_iterator = T const *;
  const_iterator cbegin() const { return ptr; }
  const_iterator cend() const { return ptr + S; }

  auto &operator[](std::size_t i) const { return ptr[i]; }
};

template <std::size_t Dim>
float dist(const Span<float, Dim> &a, const Span<float, Dim> &b) {
  float s = 0.f;
  for (auto i = 0ul; i < Dim; ++i) {
    s += (a[i] - b[i]) * (a[i] - b[i]);
  }
  return std::sqrt(s);
};

template <std::size_t Dim>
struct BuildEdgesKDTree {
  static torch::Tensor invoke(torch::Tensor &embedFeatures, float rVal,
                              int kVal) {
    assert(embedFeatures.size(1) == Dim);
    embedFeatures = embedFeatures.to(torch::kCPU);

    ////////////////
    // Build tree //
    ////////////////
    using KDTree = Acts::KDTree<Dim, int, float, Span>;

    typename KDTree::vector_t features;
    features.reserve(embedFeatures.size(0));

    auto dataPtr = embedFeatures.data_ptr<float>();

    for (int i = 0; i < embedFeatures.size(0); ++i) {
      features.push_back({Span<float, Dim>{dataPtr + i * Dim}, i});
    }

    KDTree tree(std::move(features));

    /////////////////
    // Search tree //
    /////////////////
    std::vector<std::int32_t> edges;
    edges.reserve(2 * kVal * embedFeatures.size(0));

    for (int iself = 0; iself < embedFeatures.size(0); ++iself) {
      const Span<float, Dim> self{dataPtr + iself * Dim};

      Acts::RangeXD<Dim, float> range;
      for (auto j = 0ul; j < Dim; ++j) {
        range[j] = Acts::Range1D<float>(self[j] - rVal, self[j] + rVal);
      }

      tree.rangeSearchMapDiscard(
          range, [&](const Span<float, Dim> &other, const int &iother) {
            if (iself != iother && dist(self, other) <= rVal) {
              edges.push_back(iself);
              edges.push_back(iother);
            }
          });
    }

    // Transpose is necessary here, clone to get ownership
    return Acts::detail::vectorToTensor2D(edges, 2).t().clone();
  }
};

torch::Tensor Acts::detail::buildEdgesKDTree(torch::Tensor &embedFeatures,
                                             float rVal, int kVal,
                                             bool flipDirections) {
  auto tensor = Acts::template_switch<BuildEdgesKDTree, 1, 12>(
      embedFeatures.size(1), embedFeatures, rVal, kVal);

  return postprocessEdgeTensor(tensor, true, true, flipDirections);
}

torch::Tensor Acts::detail::buildEdges(torch::Tensor &embedFeatures, float rVal,
                                       int kVal, bool flipDirections) {
#ifndef ACTS_EXATRKX_CPUONLY
  if (torch::cuda::is_available()) {
    return detail::buildEdgesFRNN(embedFeatures, rVal, kVal, flipDirections);
  } else {
    return detail::buildEdgesKDTree(embedFeatures, rVal, kVal, flipDirections);
  }
#else
  return detail::buildEdgesKDTree(embedFeatures, rVal, kVal, flipDirections);
#endif
}
