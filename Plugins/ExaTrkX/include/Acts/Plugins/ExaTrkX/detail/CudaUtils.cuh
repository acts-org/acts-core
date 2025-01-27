// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
