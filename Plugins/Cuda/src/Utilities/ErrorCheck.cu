// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
