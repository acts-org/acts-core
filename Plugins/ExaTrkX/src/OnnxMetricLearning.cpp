// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ExaTrkX/OnnxMetricLearning.hpp"

#include "Acts/Plugins/ExaTrkX/detail/buildEdges.hpp"

#include <onnxruntime_cxx_api.h>
#include <torch/script.h>

#include "runSessionWithIoBinding.hpp"

namespace Acts {

OnnxMetricLearning::OnnxMetricLearning(const Config& cfg,
                                       std::unique_ptr<const Logger> logger)
    : m_logger(std::move(logger)),
      m_cfg(cfg),
      m_device(torch::Device(torch::kCPU)) {
  m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING,
                                     "ExaTrkX - metric learning");

  Ort::SessionOptions session_options;
  session_options.SetIntraOpNumThreads(1);
  session_options.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

  m_model = std::make_unique<Ort::Session>(*m_env, m_cfg.modelPath.c_str(),
                                           session_options);
}

OnnxMetricLearning::~OnnxMetricLearning() {}

void OnnxMetricLearning::buildEdgesWrapper(std::vector<float>& embedFeatures,
                                           std::vector<std::int64_t>& edgeList,
                                           std::int64_t numSpacepoints,
                                           const Logger& logger) const {
  torch::Device device(torch::kCUDA);
  auto options =
      torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCUDA);

  torch::Tensor embedTensor =
      torch::tensor(embedFeatures, options)
          .reshape({numSpacepoints, m_cfg.embeddingDim});

  auto stackedEdges = detail::buildEdges(embedTensor, m_cfg.rVal, m_cfg.knnVal);

  stackedEdges = stackedEdges.toType(torch::kInt64).to(torch::kCPU);

  ACTS_VERBOSE("copy edges to std::vector");
  std::copy(stackedEdges.data_ptr<std::int64_t>(),
            stackedEdges.data_ptr<std::int64_t>() + stackedEdges.numel(),
            std::back_inserter(edgeList));
}

std::tuple<std::any, std::any> OnnxMetricLearning::operator()(
    std::vector<float>& inputValues, std::size_t, torch::Device) {
  Ort::AllocatorWithDefaultOptions allocator;
  auto memoryInfo = Ort::MemoryInfo::CreateCpu(
      OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

  // ************
  // Embedding
  // ************

  std::int64_t numSpacepoints = inputValues.size() / m_cfg.spacepointFeatures;
  std::vector<std::int64_t> eInputShape{numSpacepoints,
                                        m_cfg.spacepointFeatures};

  std::vector<const char*> eInputNames{"sp_features"};
  std::vector<Ort::Value> eInputTensor;
  eInputTensor.push_back(Ort::Value::CreateTensor<float>(
      memoryInfo, inputValues.data(), inputValues.size(), eInputShape.data(),
      eInputShape.size()));

  std::vector<float> eOutputData(numSpacepoints * m_cfg.embeddingDim);
  std::vector<const char*> eOutputNames{"embedding_output"};
  std::vector<std::int64_t> eOutputShape{numSpacepoints, m_cfg.embeddingDim};
  std::vector<Ort::Value> eOutputTensor;
  eOutputTensor.push_back(Ort::Value::CreateTensor<float>(
      memoryInfo, eOutputData.data(), eOutputData.size(), eOutputShape.data(),
      eOutputShape.size()));
  runSessionWithIoBinding(*m_model, eInputNames, eInputTensor, eOutputNames,
                          eOutputTensor);

  ACTS_VERBOSE("Embedding space of the first SP: ");
  for (std::size_t i = 0; i < 3; i++) {
    ACTS_VERBOSE("\t" << eOutputData[i]);
  }

  // ************
  // Building Edges
  // ************
  std::vector<std::int64_t> edgeList;
  buildEdgesWrapper(eOutputData, edgeList, numSpacepoints, logger());
  std::int64_t numEdges = edgeList.size() / 2;
  ACTS_DEBUG("Graph construction: built " << numEdges << " edges.");

  for (std::size_t i = 0; i < 10; i++) {
    ACTS_VERBOSE(edgeList[i]);
  }
  for (std::size_t i = 0; i < 10; i++) {
    ACTS_VERBOSE(edgeList[numEdges + i]);
  }

  return {std::make_shared<Ort::Value>(std::move(eInputTensor[0])), edgeList};
}

}  // namespace Acts
