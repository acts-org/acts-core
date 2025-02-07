// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <sstream>

#include <cuda_runtime_api.h>

namespace Acts::detail {

inline void cudaAssert(cudaError_t code, const char *file, int line) {
  if (code != cudaSuccess) {
    std::stringstream ss;
    ss << "CUDA error: " << cudaGetErrorString(code) << ", " << file << ":"
       << line;
    throw std::runtime_error(ss.str());
  }
}

}  // namespace Acts::detail

#define ACTS_CUDA_CHECK(ans)                             \
  do {                                                   \
    Acts::detail::cudaAssert((ans), __FILE__, __LINE__); \
  } while (0)
