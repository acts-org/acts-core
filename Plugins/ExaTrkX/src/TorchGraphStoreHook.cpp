// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ExaTrkX/TorchGraphStoreHook.hpp"

#include "Acts/Plugins/ExaTrkX/detail/TensorVectorConversion.hpp"

#include <torch/torch.h>

Acts::TorchGraphStoreHook::TorchGraphStoreHook() {
  m_storedGraph = std::make_unique<Graph>();
}

void Acts::TorchGraphStoreHook::operator()(const std::any&,
                                           const std::any& edges,
                                           const std::any& weights) const {
  if (not weights.has_value()) {
    return;
  }

  m_storedGraph->first = detail::tensor2DToVector<std::int64_t>(
      std::any_cast<torch::Tensor>(edges).t());

  auto cpuWeights = std::any_cast<torch::Tensor>(weights).to(torch::kCPU);
  m_storedGraph->second =
      std::vector<float>(cpuWeights.data_ptr<float>(),
                         cpuWeights.data_ptr<float>() + cpuWeights.numel());
}
