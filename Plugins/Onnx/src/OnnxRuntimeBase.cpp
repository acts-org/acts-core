// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Onnx/OnnxRuntimeBase.hpp"

#include <cassert>
#include <stdexcept>

// Parametrized constructor
Acts::OnnxRuntimeBase::OnnxRuntimeBase(Ort::Env& env, const char* modelPath) {
  // Set the ONNX runtime session options
  Ort::SessionOptions sessionOptions;
  // Set graph optimization level
  sessionOptions.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_ENABLE_BASIC);
  // Create the Ort session
  m_session = std::make_unique<Ort::Session>(env, modelPath, sessionOptions);
  // Default allocator
  Ort::AllocatorWithDefaultOptions allocator;

  // Get the names of the input nodes of the model
  std::size_t numInputNodes = m_session->GetInputCount();
  // Iterate over all input nodes and get the name
  for (std::size_t i = 0; i < numInputNodes; i++) {
    m_inputNodeNamesAllocated.push_back(
        m_session->GetInputNameAllocated(i, allocator));
    m_inputNodeNames.push_back(m_inputNodeNamesAllocated.back().get());

    // Get the dimensions of the input nodes
    // Assumes single input
    Ort::TypeInfo inputTypeInfo = m_session->GetInputTypeInfo(i);
    auto tensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
    m_inputNodeDims = tensorInfo.GetShape();
  }

  // Get the names of the output nodes
  std::size_t numOutputNodes = m_session->GetOutputCount();
  // Iterate over all output nodes and get the name
  for (std::size_t i = 0; i < numOutputNodes; i++) {
    m_outputNodeNamesAllocated.push_back(
        m_session->GetOutputNameAllocated(i, allocator));
    m_outputNodeNames.push_back(m_outputNodeNamesAllocated.back().get());

    // Get the dimensions of the output nodes
    Ort::TypeInfo outputTypeInfo = m_session->GetOutputTypeInfo(i);
    auto tensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
    m_outputNodeDims.push_back(tensorInfo.GetShape());
  }
}

// Inference function using ONNX runtime for one single entry
std::vector<float> Acts::OnnxRuntimeBase::runONNXInference(
    std::vector<float>& inputTensorValues) const {
  Acts::NetworkBatchInput vectorInput(1, inputTensorValues.size());
  for (std::size_t i = 0; i < inputTensorValues.size(); i++) {
    vectorInput(0, i) = inputTensorValues[i];
  }
  auto vectorOutput = runONNXInference(vectorInput);
  return vectorOutput[0];
}

// Inference function using ONNX runtime
// the function assumes that the model has 1 input node and 1 output node
std::vector<std::vector<float>> Acts::OnnxRuntimeBase::runONNXInference(
    Acts::NetworkBatchInput& inputTensorValues) const {
  return runONNXInferenceMultiOutput(inputTensorValues).front();
}

// Inference function for single-input, multi-output models
std::vector<std::vector<std::vector<float>>>
Acts::OnnxRuntimeBase::runONNXInferenceMultiOutput(
    NetworkBatchInput& inputTensorValues) const {
  int batchSize = inputTensorValues.rows();
  std::vector<std::int64_t> inputNodeDims = m_inputNodeDims;
  std::vector<std::vector<std::int64_t>> outputNodeDims = m_outputNodeDims;

  // The first dim node should correspond to the batch size
  // If it is -1, it is dynamic and should be set to the input size
  if (inputNodeDims[0] == -1) {
    inputNodeDims[0] = batchSize;
  }

  bool outputDimsMatch = true;
  for (std::vector<std::int64_t>& nodeDim : outputNodeDims) {
    if (nodeDim[0] == -1) {
      nodeDim[0] = batchSize;
    }
    outputDimsMatch &= batchSize == 1 || nodeDim[0] == batchSize;
  }

  if (batchSize != 1 && (inputNodeDims[0] != batchSize || !outputDimsMatch)) {
    throw std::runtime_error(
        "runONNXInference: batch size doesn't match the input or output node "
        "size");
  }

  // Create input tensor object from data values
  Ort::MemoryInfo memoryInfo =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
      memoryInfo, inputTensorValues.data(), inputTensorValues.size(),
      inputNodeDims.data(), inputNodeDims.size());
  // Double-check that inputTensor is a Tensor
  if (!inputTensor.IsTensor()) {
    throw std::runtime_error(
        "runONNXInference: conversion of input to Tensor failed. ");
  }
  // Score model on input tensors, get back output tensors
  Ort::RunOptions run_options;
  std::vector<Ort::Value> outputTensors =
      m_session->Run(run_options, m_inputNodeNames.data(), &inputTensor,
                     m_inputNodeNames.size(), m_outputNodeNames.data(),
                     m_outputNodeNames.size());

  // Double-check that outputTensors contains Tensors and that the count matches
  // that of output nodes
  if (!outputTensors[0].IsTensor() ||
      (outputTensors.size() != m_outputNodeNames.size())) {
    throw std::runtime_error(
        "runONNXInference: calculation of output failed. ");
  }

  std::vector<std::vector<std::vector<float>>> multiOutput;

  for (std::size_t i_out = 0; i_out < outputTensors.size(); i_out++) {
    // Get pointer to output tensor float values
    float* outputTensor = outputTensors.at(i_out).GetTensorMutableData<float>();
    // Get the output values
    std::vector<std::vector<float>> outputTensorValues(
        batchSize, std::vector<float>(outputNodeDims.at(i_out)[1], -1));
    for (int i = 0; i < outputNodeDims.at(i_out)[0]; i++) {
      for (int j = 0; j < ((outputNodeDims.at(i_out).size() > 1)
                               ? outputNodeDims.at(i_out)[1]
                               : 1);
           j++) {
        outputTensorValues[i][j] =
            outputTensor[i * outputNodeDims.at(i_out)[1] + j];
      }
    }
    multiOutput.push_back(std::move(outputTensorValues));
  }
  return multiOutput;
}
