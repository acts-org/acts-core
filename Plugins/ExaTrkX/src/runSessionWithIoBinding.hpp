// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
