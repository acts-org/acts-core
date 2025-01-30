// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ExaTrkX/TensorRTEdgeClassifier.hpp"

#include "Acts/Plugins/ExaTrkX/detail/CudaUtils.cuh"
#include "Acts/Plugins/ExaTrkX/detail/Utils.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

#include <NvInfer.h>
#include <NvInferPlugin.h>
#include <NvInferRuntimeBase.h>
#include <c10/cuda/CUDAGuard.h>
#include <cuda_runtime.h>

#include "printCudaMemInfo.hpp"

using namespace torch::indexing;

namespace {

class TensorRTLogger : public nvinfer1::ILogger {
  std::unique_ptr<const Acts::Logger> m_logger;

 public:
  TensorRTLogger(Acts::Logging::Level lvl)
      : m_logger(Acts::getDefaultLogger("TensorRT", lvl)) {}

  void log(Severity severity, const char *msg) noexcept override {
    const auto &logger = *m_logger;
    switch (severity) {
      case Severity::kVERBOSE:
        ACTS_DEBUG(msg);
        break;
      case Severity::kINFO:
        ACTS_INFO(msg);
        break;
      case Severity::kWARNING:
        ACTS_WARNING(msg);
        break;
      case Severity::kERROR:
        ACTS_ERROR(msg);
        break;
      case Severity::kINTERNAL_ERROR:
        ACTS_FATAL(msg);
        break;
    }
  }
};

}  // namespace

namespace Acts {

TensorRTEdgeClassifier::TensorRTEdgeClassifier(
    const Config &cfg, std::unique_ptr<const Logger> _logger)
    : m_logger(std::move(_logger)),
      m_cfg(cfg),
      m_trtLogger(std::make_unique<TensorRTLogger>(m_logger->level())) {
  auto status = initLibNvInferPlugins(m_trtLogger.get(), "");
  if (!status) {
    throw std::runtime_error("Failed to initialize TensorRT plugins");
  }

  std::size_t fsize =
      std::filesystem::file_size(std::filesystem::path(m_cfg.modelPath));
  std::vector<char> engineData(fsize);

  ACTS_DEBUG("Load '" << m_cfg.modelPath << "' with size " << fsize);

  std::ifstream engineFile(m_cfg.modelPath);
  if (!engineFile) {
    throw std::runtime_error("Failed to open engine file");
  } else if (!engineFile.read(engineData.data(), fsize)) {
    throw std::runtime_error("Failed to read engine file");
  }

  m_runtime.reset(nvinfer1::createInferRuntime(*m_trtLogger));
  if (!m_runtime) {
    throw std::runtime_error("Failed to create TensorRT runtime");
  }

  m_engine.reset(m_runtime->deserializeCudaEngine(engineData.data(), fsize));
  if (!m_engine) {
    throw std::runtime_error("Failed to deserialize CUDA engine");
  }

  for (auto i = 0ul; i < m_cfg.numExecutionContexts; ++i) {
    ACTS_DEBUG("Create execution context " << i);
    m_contexts.emplace_back(m_engine->createExecutionContext());
    if (!m_contexts.back()) {
      throw std::runtime_error("Failed to create execution context");
    }
  }

  std::size_t freeMem{}, totalMem{};
  ACTS_CUDA_CHECK(cudaMemGetInfo(&freeMem, &totalMem));
  ACTS_DEBUG("Used CUDA memory after TensorRT initialization: "
             << (totalMem - freeMem) * 1e-9 << " / " << totalMem * 1e-9
             << " GB");
}

TensorRTEdgeClassifier::~TensorRTEdgeClassifier() {}

std::tuple<std::any, std::any, std::any, std::any>
TensorRTEdgeClassifier::operator()(std::any inNodeFeatures,
                                   std::any inEdgeIndex,
                                   std::any inEdgeFeatures,
                                   const ExecutionContext &execContext) {
  assert(execContext.device.is_cuda());
  decltype(std::chrono::high_resolution_clock::now()) t0, t1, t2, t3, t4;
  t0 = std::chrono::high_resolution_clock::now();

  c10::cuda::CUDAStreamGuard(execContext.stream.value());

  auto nodeFeatures =
      std::any_cast<torch::Tensor>(inNodeFeatures).to(execContext.device);

  auto edgeIndex =
      std::any_cast<torch::Tensor>(inEdgeIndex).to(execContext.device);
  ACTS_DEBUG("edgeIndex: " << detail::TensorDetails{edgeIndex});

  auto edgeFeatures =
      std::any_cast<torch::Tensor>(inEdgeFeatures).to(execContext.device);
  ACTS_DEBUG("edgeFeatures: " << detail::TensorDetails{edgeFeatures});

  t1 = std::chrono::high_resolution_clock::now();

  // get a context from the list of contexts
  std::unique_ptr<nvinfer1::IExecutionContext> context;
  while (context == nullptr) {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    if (!m_contexts.empty()) {
      context = std::move(m_contexts.back());
      m_contexts.pop_back();
    }
  }
  assert(context != nullptr);

  context->setInputShape(
      "x", nvinfer1::Dims2{nodeFeatures.size(0), nodeFeatures.size(1)});
  context->setTensorAddress("x", nodeFeatures.data_ptr());

  context->setInputShape("edge_index",
                         nvinfer1::Dims2{edgeIndex.size(0), edgeIndex.size(1)});
  context->setTensorAddress("edge_index", edgeIndex.data_ptr());

  context->setInputShape(
      "edge_attr", nvinfer1::Dims2{edgeFeatures.size(0), edgeFeatures.size(1)});
  context->setTensorAddress("edge_attr", edgeFeatures.data_ptr());

  auto scores = torch::empty(
      edgeIndex.size(1),
      torch::TensorOptions().device(torch::kCUDA).dtype(torch::kFloat32));
  context->setTensorAddress("output", scores.data_ptr());

  t2 = std::chrono::high_resolution_clock::now();

  auto stream = execContext.stream.value().stream();
  auto status = context->enqueueV3(stream);
  if (!status) {
    throw std::runtime_error("Failed to execute TensorRT model");
  }
  ACTS_CUDA_CHECK(cudaStreamSynchronize(stream));

  t3 = std::chrono::high_resolution_clock::now();

  {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    m_contexts.push_back(std::move(context));
  }

  scores.sigmoid_();

  ACTS_VERBOSE("Size after classifier: " << scores.size(0));
  ACTS_VERBOSE("Slice of classified output:\n"
               << scores.slice(/*dim=*/0, /*start=*/0, /*end=*/9));
  printCudaMemInfo(logger());

  torch::Tensor mask = scores > m_cfg.cut;
  torch::Tensor edgesAfterCut = edgeIndex.index({Slice(), mask});

  scores = scores.masked_select(mask);
  ACTS_VERBOSE("Size after score cut: " << edgesAfterCut.size(1));
  printCudaMemInfo(logger());

  t4 = std::chrono::high_resolution_clock::now();

  auto milliseconds = [](const auto &a, const auto &b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
  };
  ACTS_DEBUG("Time anycast:  " << milliseconds(t0, t1));
  ACTS_DEBUG("Time alloc, set shape " << milliseconds(t1, t2));
  ACTS_DEBUG("Time inference:       " << milliseconds(t2, t3));
  ACTS_DEBUG("Time sigmoid and cut: " << milliseconds(t3, t4));

  return {nodeFeatures, edgesAfterCut, edgeFeatures, scores};
}

}  // namespace Acts
