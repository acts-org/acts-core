// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
  if (torch::cuda::is_available() && logger.level() == Acts::Logging::VERBOSE) {
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
