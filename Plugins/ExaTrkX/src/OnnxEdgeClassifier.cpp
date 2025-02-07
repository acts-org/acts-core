// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/ExaTrkX/OnnxEdgeClassifier.hpp"

#include "Acts/Plugins/ExaTrkX/detail/Utils.hpp"

#include <onnxruntime_cxx_api.h>
#include <torch/script.h>

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
  // session_options.SetGraphOptimizationLevel(
  //     GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

  OrtCUDAProviderOptions cuda_options;
  cuda_options.device_id = 0;
  // session_options.AppendExecutionProvider_CUDA(cuda_options);

  m_model = std::make_unique<Ort::Session>(*m_env, m_cfg.modelPath.c_str(),
                                           session_options);

  Ort::AllocatorWithDefaultOptions allocator;

  for (std::size_t i = 0; i < m_model->GetInputCount(); ++i) {
    m_inputNames.emplace_back(
        m_model->GetInputNameAllocated(i, allocator).get());
  }
  m_outputName =
      std::string(m_model->GetOutputNameAllocated(0, allocator).get());
}

OnnxEdgeClassifier::~OnnxEdgeClassifier() {}

template <typename T>
auto torchToOnnx(Ort::MemoryInfo &memInfo, at::Tensor &tensor) {
  std::vector<std::int64_t> shape{tensor.size(0), tensor.size(1)};
  return Ort::Value::CreateTensor<T>(memInfo, tensor.data_ptr<T>(),
                                     tensor.numel(), shape.data(),
                                     shape.size());
}

std::ostream &operator<<(std::ostream &os, Ort::Value &v) {
  if (!v.IsTensor()) {
    os << "no tensor";
    return os;
  }

  auto shape = v.GetTensorTypeAndShapeInfo().GetShape();

  auto printVal = [&]<typename T>() {
    for (int i = 0; i < shape.at(0); ++i) {
      for (int j = 0; j < shape.at(1); ++j) {
        os << v.At<T>({i, j}) << " ";
      }
      os << "\n";
    }
  };

  auto type = v.GetTensorTypeAndShapeInfo().GetElementType();
  if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
    os << "[float tensor]\n";
    printVal.operator()<float>();
  } else if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64) {
    os << "[int64 tensor]\n";
    printVal.operator()<std::int64_t>();
  } else {
    os << "not implemented datatype";
  }

  return os;
}

std::tuple<std::any, std::any, std::any, std::any>
OnnxEdgeClassifier::operator()(std::any inputNodes, std::any inputEdges,
                               std::any inEdgeFeatures,
                               const ExecutionContext & /*unused*/) {
  auto torchDevice = torch::kCPU;
  Ort::MemoryInfo memoryInfo("Cpu", OrtArenaAllocator, /*device_id*/ 0,
                             OrtMemTypeDefault);

  Ort::Allocator allocator(*m_model, memoryInfo);

  auto nodeTensor =
      std::any_cast<torch::Tensor>(inputNodes).to(torchDevice).clone();
  auto edgeList = std::any_cast<torch::Tensor>(inputEdges).to(torchDevice);
  const int numEdges = edgeList.size(1);

  std::vector<const char *> inputNames{m_inputNames.at(0).c_str(),
                                       m_inputNames.at(1).c_str()};

  // TODO move this contiguous to graph construction
  auto edgeListClone = edgeList.clone().contiguous();
  ACTS_DEBUG("edgeIndex: " << detail::TensorDetails{edgeListClone});
  auto nodeTensorClone = nodeTensor.clone();
  ACTS_DEBUG("nodes: " << detail::TensorDetails{nodeTensorClone});
  std::vector<Ort::Value> inputTensors;
  inputTensors.push_back(torchToOnnx<float>(memoryInfo, nodeTensorClone));
  inputTensors.push_back(torchToOnnx<std::int64_t>(memoryInfo, edgeListClone));

  std::optional<at::Tensor> edgeAttrTensor;
  if (inEdgeFeatures.has_value()) {
    inputNames.push_back(m_inputNames.at(2).c_str());
    edgeAttrTensor =
        std::any_cast<torch::Tensor>(inEdgeFeatures).to(torchDevice).clone();
    inputTensors.push_back(torchToOnnx<float>(memoryInfo, *edgeAttrTensor));
  }

  std::vector<const char *> outputNames{m_outputName.c_str()};

  auto outputTensor =
      m_model->Run({}, inputNames.data(), inputTensors.data(),
                   inputTensors.size(), outputNames.data(), outputNames.size());

  float *rawOutData = nullptr;
  if (outputTensor.at(0).GetTensorTypeAndShapeInfo().GetElementType() ==
      ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
    rawOutData = outputTensor.at(0).GetTensorMutableData<float>();
  } else {
    throw std::runtime_error("Invalid output datatype");
  }

  ACTS_DEBUG("Get scores for " << numEdges << " edges.");
  auto scores =
      torch::from_blob(
          rawOutData, {numEdges},
          torch::TensorOptions().device(torchDevice).dtype(torch::kFloat32))
          .clone();

  ACTS_VERBOSE("Slice of classified output before sigmoid:\n"
               << scores.slice(/*dim=*/0, /*start=*/0, /*end=*/9));

  scores.sigmoid_();

  ACTS_DEBUG("scores: " << detail::TensorDetails{scores});
  ACTS_VERBOSE("Slice of classified output:\n"
               << scores.slice(/*dim=*/0, /*start=*/0, /*end=*/9));

  torch::Tensor filterMask = scores > m_cfg.cut;
  torch::Tensor edgesAfterCut = edgeList.index({Slice(), filterMask});

  ACTS_DEBUG("Finished edge classification, after cut: "
             << edgesAfterCut.size(1) << " edges.");

  if (edgesAfterCut.size(1) == 0) {
    throw Acts::NoEdgesError{};
  }

  return {std::move(nodeTensor), edgesAfterCut.clone(),
          std::move(inEdgeFeatures), std::move(scores)};
}

}  // namespace Acts
