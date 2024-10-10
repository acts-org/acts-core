// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ExaTrkX/OnnxEdgeClassifier.hpp"

#include <onnxruntime_cxx_api.h>
#include <torch/script.h>

#include "runSessionWithIoBinding.hpp"

using namespace torch::indexing;

namespace Acts {

OnnxEdgeClassifier::OnnxEdgeClassifier(const Config &cfg,
                                       std::unique_ptr<const Logger> logger)
    : m_logger(std::move(logger)),
      m_cfg(cfg),
      m_device(torch::Device(torch::kCPU)) {
  m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING,
                                     "ExaTrkX - edge classifier");

  Ort::SessionOptions session_options;
  session_options.SetIntraOpNumThreads(1);
  session_options.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

  m_model = std::make_unique<Ort::Session>(*m_env, m_cfg.modelPath.c_str(),
                                           session_options);

  Ort::AllocatorWithDefaultOptions allocator;

  m_inputNameNodes =
      std::string(m_model->GetInputNameAllocated(0, allocator).get());
  m_inputNameEdges =
      std::string(m_model->GetInputNameAllocated(1, allocator).get());
  m_outputNameScores =
      std::string(m_model->GetOutputNameAllocated(0, allocator).get());
}

OnnxEdgeClassifier::~OnnxEdgeClassifier() {}

std::tuple<std::any, std::any, std::any> OnnxEdgeClassifier::operator()(
    std::any inputNodes, std::any inputEdges, torch::Device) {
  Ort::AllocatorWithDefaultOptions allocator;
  auto memoryInfo = Ort::MemoryInfo::CreateCpu(
      OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

  auto eInputTensor = std::any_cast<std::shared_ptr<Ort::Value>>(inputNodes);
  auto edgeList = std::any_cast<std::vector<std::int64_t>>(inputEdges);
  const int numEdges = edgeList.size() / 2;

  std::vector<const char *> fInputNames{m_inputNameNodes.c_str(),
                                        m_inputNameEdges.c_str()};
  std::vector<Ort::Value> fInputTensor;
  fInputTensor.push_back(std::move(*eInputTensor));
  std::vector<std::int64_t> fEdgeShape{2, numEdges};
  fInputTensor.push_back(Ort::Value::CreateTensor<std::int64_t>(
      memoryInfo, edgeList.data(), edgeList.size(), fEdgeShape.data(),
      fEdgeShape.size()));

  // filtering outputs
  std::vector<const char *> fOutputNames{m_outputNameScores.c_str()};
  std::vector<float> fOutputData(numEdges);

  auto outputDims = m_model->GetOutputTypeInfo(0)
                        .GetTensorTypeAndShapeInfo()
                        .GetDimensionsCount();
  using Shape = std::vector<std::int64_t>;
  Shape fOutputShape = outputDims == 2 ? Shape{numEdges, 1} : Shape{numEdges};
  std::vector<Ort::Value> fOutputTensor;
  fOutputTensor.push_back(Ort::Value::CreateTensor<float>(
      memoryInfo, fOutputData.data(), fOutputData.size(), fOutputShape.data(),
      fOutputShape.size()));
  runSessionWithIoBinding(*m_model, fInputNames, fInputTensor, fOutputNames,
                          fOutputTensor);

  ACTS_DEBUG("Get scores for " << numEdges << " edges.");
  torch::Tensor edgeListCTen = torch::tensor(edgeList, {torch::kInt64});
  edgeListCTen = edgeListCTen.reshape({2, numEdges});

  torch::Tensor fOutputCTen = torch::tensor(fOutputData, {torch::kFloat32});
  fOutputCTen = fOutputCTen.sigmoid();

  torch::Tensor filterMask = fOutputCTen > m_cfg.cut;
  torch::Tensor edgesAfterFCTen = edgeListCTen.index({Slice(), filterMask});

  std::vector<std::int64_t> edgesAfterFiltering;
  std::copy(edgesAfterFCTen.data_ptr<std::int64_t>(),
            edgesAfterFCTen.data_ptr<std::int64_t>() + edgesAfterFCTen.numel(),
            std::back_inserter(edgesAfterFiltering));

  std::int64_t numEdgesAfterF = edgesAfterFiltering.size() / 2;
  ACTS_DEBUG("Finished edge classification, after cut: " << numEdgesAfterF
                                                         << " edges.");

  return {std::make_shared<Ort::Value>(std::move(fInputTensor[0])),
          edgesAfterFiltering, fOutputCTen};
}

}  // namespace Acts
