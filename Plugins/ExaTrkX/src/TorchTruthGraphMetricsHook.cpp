// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ExaTrkX/TorchTruthGraphMetricsHook.hpp"

#include "Acts/Plugins/ExaTrkX/detail/TensorVectorConversion.hpp"

#include <algorithm>

#include <torch/torch.h>

namespace {

auto cantorize(std::vector<std::int64_t> edgeIndex,
               const Acts::Logger& logger) {
  // Use cantor pairing to store truth graph, so we can easily use set
  // operations to compute efficiency and purity
  std::vector<Acts::detail::CantorEdge<std::int64_t>> cantorEdgeIndex;
  cantorEdgeIndex.reserve(edgeIndex.size() / 2);
  for (auto it = edgeIndex.begin(); it != edgeIndex.end(); it += 2) {
    cantorEdgeIndex.emplace_back(*it, *std::next(it));
  }

  std::ranges::sort(cantorEdgeIndex,
                    std::less<Acts::detail::CantorEdge<std::int64_t>>{});

  auto new_end = std::unique(cantorEdgeIndex.begin(), cantorEdgeIndex.end());
  if (new_end != cantorEdgeIndex.end()) {
    ACTS_WARNING("Graph not unique ("
                 << std::distance(new_end, cantorEdgeIndex.end())
                 << " duplicates)");
    cantorEdgeIndex.erase(new_end, cantorEdgeIndex.end());
  }

  return cantorEdgeIndex;
}

}  // namespace

Acts::TorchTruthGraphMetricsHook::TorchTruthGraphMetricsHook(
    const std::vector<std::int64_t>& truthGraph,
    std::unique_ptr<const Acts::Logger> l)
    : m_logger(std::move(l)) {
  m_truthGraphCantor = cantorize(truthGraph, logger());
}

void Acts::TorchTruthGraphMetricsHook::operator()(const std::any&,
                                                  const std::any& edges,
                                                  const std::any&) const {
  // We need to transpose the edges here for the right memory layout
  const auto edgeIndex = Acts::detail::tensor2DToVector<std::int64_t>(
      std::any_cast<torch::Tensor>(edges).t());

  auto predGraphCantor = cantorize(edgeIndex, logger());

  // Calculate intersection
  std::vector<Acts::detail::CantorEdge<std::int64_t>> intersection;
  intersection.reserve(
      std::max(predGraphCantor.size(), m_truthGraphCantor.size()));

  std::set_intersection(predGraphCantor.begin(), predGraphCantor.end(),
                        m_truthGraphCantor.begin(), m_truthGraphCantor.end(),
                        std::back_inserter(intersection));

  ACTS_DEBUG("Intersection size " << intersection.size());
  const float intersectionSizeFloat = intersection.size();
  const float eff = intersectionSizeFloat / m_truthGraphCantor.size();
  const float pur = intersectionSizeFloat / predGraphCantor.size();

  ACTS_INFO("Efficiency=" << eff << ", purity=" << pur);
}
