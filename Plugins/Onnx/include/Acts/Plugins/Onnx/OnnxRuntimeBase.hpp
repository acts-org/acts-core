// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <vector>

#include <Eigen/Dense>
#include <onnxruntime_cxx_api.h>

namespace Acts {

using NetworkBatchInput =
    Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

// General class that sets up the ONNX runtime framework for loading an ML model
// and using it for inference.
class OnnxRuntimeBase {
 public:
  /// @brief Default constructor
  OnnxRuntimeBase() = default;

  /// @brief Parametrized constructor
  ///
  /// @param env the ONNX runtime environment
  /// @param modelPath the path to the ML model in *.onnx format
  OnnxRuntimeBase(Ort::Env& env, const char* modelPath);

  /// @brief Default destructor
  ~OnnxRuntimeBase() = default;

  /// @brief Run the ONNX inference function
  ///
  /// @param inputTensorValues The input feature values used for prediction
  ///
  /// @return The output (predicted) values
  std::vector<float> runONNXInference(
      std::vector<float>& inputTensorValues) const;

  /// @brief Run the ONNX inference function for a batch of input
  ///
  /// @param inputTensorValues Vector of the input feature values of all the inputs used for prediction
  ///
  /// @return The vector of output (predicted) values
  std::vector<std::vector<float>> runONNXInference(
      NetworkBatchInput& inputTensorValues) const;

  /// @brief Run the multi-output ONNX inference function for a batch of input
  ///
  /// @param inputTensorValues Vector of the input feature values of all the inputs used for prediction
  ///
  /// @return The vector of output (predicted) values, one for each output
  std::vector<std::vector<std::vector<float>>> runONNXInferenceMultiOutput(
      NetworkBatchInput& inputTensorValues) const;

 private:
  /// ONNX runtime session / model properties
  std::unique_ptr<Ort::Session> m_session;
  std::vector<Ort::AllocatedStringPtr> m_inputNodeNamesAllocated;
  std::vector<const char*> m_inputNodeNames;
  std::vector<std::int64_t> m_inputNodeDims;
  std::vector<Ort::AllocatedStringPtr> m_outputNodeNamesAllocated;
  std::vector<const char*> m_outputNodeNames;
  std::vector<std::vector<std::int64_t>> m_outputNodeDims;
};

}  // namespace Acts
