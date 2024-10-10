// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// CUDA include(s).
#include <cuda_runtime.h>

// System include(s).
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace Acts {
namespace Cuda {
namespace Details {

void throwError(cudaError_t errorCode, const char* expression, const char* file,
                int line) {
  // Create a nice error message.
  std::ostringstream errorMsg;
  errorMsg << file << ":" << line << " Failed to execute: " << expression
           << " (" << cudaGetErrorString(errorCode) << ")";
  // Now print and then throw it.
  std::cerr << errorMsg.str() << std::endl;
  throw std::runtime_error(errorMsg.str());
}

}  // namespace Details
}  // namespace Cuda
}  // namespace Acts
