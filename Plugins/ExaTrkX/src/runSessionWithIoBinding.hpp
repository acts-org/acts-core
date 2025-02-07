// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <onnxruntime_cxx_api.h>

inline void runSessionWithIoBinding(Ort::Session& sess,
                                    std::vector<const char*>& inputNames,
                                    std::vector<Ort::Value>& inputData,
                                    std::vector<const char*>& outputNames,
                                    std::vector<Ort::Value>& outputData) {
  if (inputNames.size() < 1) {
    throw std::runtime_error("Onnxruntime input data mapping cannot be empty");
  }
  if (inputNames.size() != inputData.size()) {
    throw std::runtime_error("inputData size mismatch");
  }

  Ort::IoBinding iobinding(sess);
  for (std::size_t idx = 0; idx < inputNames.size(); ++idx) {
    iobinding.BindInput(inputNames[idx], inputData[idx]);
  }

  for (std::size_t idx = 0; idx < outputNames.size(); ++idx) {
    iobinding.BindOutput(outputNames[idx], outputData[idx]);
  }

  sess.Run(Ort::RunOptions{nullptr}, iobinding);
}
