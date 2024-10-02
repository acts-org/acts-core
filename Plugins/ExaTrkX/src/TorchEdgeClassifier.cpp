// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ExaTrkX/TorchEdgeClassifier.hpp"

#ifndef ACTS_EXATRKX_CPUONLY
#include <c10/cuda/CUDAGuard.h>
#endif

#include <torch/script.h>
#include <torch/torch.h>

#include "printCudaMemInfo.hpp"

using namespace torch::indexing;

namespace Acts {

TorchEdgeClassifier::TorchEdgeClassifier(const Config& cfg,
                                         std::unique_ptr<const Logger> _logger)
    : m_logger(std::move(_logger)),
      m_cfg(cfg),
      m_device(torch::Device(torch::kCPU)) {
  c10::InferenceMode guard(true);
  m_deviceType = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
  if (m_deviceType == torch::kCPU) {
    ACTS_DEBUG("Running on CPU...");
  } else {
    if (cfg.deviceID >= 0 &&
        static_cast<std::size_t>(cfg.deviceID) < torch::cuda::device_count()) {
      ACTS_DEBUG("GPU device " << cfg.deviceID << " is being used.");
      m_device = torch::Device(torch::kCUDA, cfg.deviceID);
    } else {
      ACTS_WARNING("GPU device " << cfg.deviceID
                                 << " not available, falling back to CPU.");
    }
  }

  ACTS_DEBUG("Using torch version " << TORCH_VERSION_MAJOR << "."
                                    << TORCH_VERSION_MINOR << "."
                                    << TORCH_VERSION_PATCH);
#ifndef ACTS_EXATRKX_CPUONLY
  if (not torch::cuda::is_available()) {
    ACTS_INFO("CUDA not available, falling back to CPU");
  }
#endif

  try {
    m_model = std::make_unique<torch::jit::Module>();
    *m_model = torch::jit::load(m_cfg.modelPath.c_str(), m_device);
    m_model->eval();
  } catch (const c10::Error& e) {
    throw std::invalid_argument("Failed to load models: " + e.msg());
  }
}

TorchEdgeClassifier::~TorchEdgeClassifier() {}

std::tuple<std::any, std::any, std::any> TorchEdgeClassifier::operator()(
    std::any inputNodes, std::any inputEdges, torch::Device device) {
  ACTS_DEBUG("Start edge classification");
  c10::InferenceMode guard(true);

  // add a protection to avoid calling for kCPU
#ifndef ACTS_EXATRKX_CPUONLY
  std::optional<c10::cuda::CUDAGuard> device_guard;
  if (device.is_cuda()) {
    device_guard.emplace(device.index());
  }
#endif

  auto nodes = std::any_cast<torch::Tensor>(inputNodes).to(device);
  auto edgeList = std::any_cast<torch::Tensor>(inputEdges).to(device);

  auto model = m_model->clone();
  model.to(device);

  if (m_cfg.numFeatures > nodes.size(1)) {
    throw std::runtime_error("requested more features then available");
  }

  torch::Tensor output;

  // Scope this to keep inference objects separate
  {
    auto edgeListTmp = m_cfg.undirected
                           ? torch::cat({edgeList, edgeList.flip(0)}, 1)
                           : edgeList;

    std::vector<torch::jit::IValue> inputTensors(2);
    inputTensors[0] =
        m_cfg.numFeatures < nodes.size(1)
            ? nodes.index({Slice{}, Slice{None, m_cfg.numFeatures}})
            : nodes;

    if (m_cfg.nChunks > 1) {
      std::vector<at::Tensor> results;
      results.reserve(m_cfg.nChunks);

      auto chunks = at::chunk(edgeListTmp, m_cfg.nChunks, 1);
      for (auto& chunk : chunks) {
        ACTS_VERBOSE("Process chunk with shape" << chunk.sizes());
        inputTensors[1] = chunk;

        results.push_back(model.forward(inputTensors).toTensor());
        results.back().squeeze_();
      }

      output = torch::cat(results);
    } else {
      inputTensors[1] = edgeListTmp;
      output = model.forward(inputTensors).toTensor();
      output.squeeze_();
    }
  }

  output.sigmoid_();

  if (m_cfg.undirected) {
    auto newSize = output.size(0) / 2;
    output = output.index({Slice(None, newSize)});
  }

  ACTS_VERBOSE("Size after classifier: " << output.size(0));
  ACTS_VERBOSE("Slice of classified output:\n"
               << output.slice(/*dim=*/0, /*start=*/0, /*end=*/9));
  printCudaMemInfo(logger());

  torch::Tensor mask = output > m_cfg.cut;
  torch::Tensor edgesAfterCut = edgeList.index({Slice(), mask});
  edgesAfterCut = edgesAfterCut.to(torch::kInt64);

  ACTS_VERBOSE("Size after score cut: " << edgesAfterCut.size(1));
  printCudaMemInfo(logger());

  return {std::move(nodes), std::move(edgesAfterCut),
          output.masked_select(mask)};
}

}  // namespace Acts
