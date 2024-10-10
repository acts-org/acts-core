// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <Acts/Utilities/Logger.hpp>

#ifndef ACTS_EXATRKX_CPUONLY
#include <cuda_runtime_api.h>
#endif

#include <cstdint>

#include <torch/torch.h>

namespace {

inline void printCudaMemInfo(const Acts::Logger& logger) {
#ifndef ACTS_EXATRKX_CPUONLY
  if (torch::cuda::is_available()) {
    constexpr float kb = 1024;
    constexpr float mb = kb * kb;

    int device;
    std::size_t free, total;
    cudaMemGetInfo(&free, &total);
    cudaGetDevice(&device);

    ACTS_VERBOSE("Current CUDA device: " << device);
    ACTS_VERBOSE("Memory (used / total) [in MB]: " << (total - free) / mb
                                                   << " / " << total / mb);
  } else {
    ACTS_VERBOSE("No memory info, CUDA disabled");
  }
#else
  ACTS_VERBOSE("No memory info, CUDA disabled");
#endif
}

}  // namespace
