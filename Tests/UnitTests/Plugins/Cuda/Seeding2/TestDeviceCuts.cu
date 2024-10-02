// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Local include(s).
#include "TestDeviceCuts.hpp"

// CUDA include(s).
#include <cuda_runtime.h>

// System include(s).
#include <stdexcept>

/// Helper macro for checking the return values of CUDA function calls
#define CUDA_CHECK(EXP)                                         \
  do {                                                          \
    cudaError_t errorCode = EXP;                                \
    if (errorCode != cudaSuccess) {                             \
      throw std::runtime_error(                                 \
          "Failed to set up the custom seed filter functions"); \
    }                                                           \
  } while (false)

/// Code mimicking @c TestHostCuts::seedWeight
__device__ float testSeedWeight(const Acts::Cuda::Details::SpacePoint& bottom,
                                const Acts::Cuda::Details::SpacePoint&,
                                const Acts::Cuda::Details::SpacePoint& top) {
  float weight = 0;
  if (bottom.radius > 150) {
    weight = 400;
  }
  if (top.radius < 150) {
    weight = 200;
  }
  return weight;
}

/// Code mimicking @c TestHostCuts::singleSeedCut
__device__ bool testSingleSeedCut(float weight,
                                  const Acts::Cuda::Details::SpacePoint& bottom,
                                  const Acts::Cuda::Details::SpacePoint&,
                                  const Acts::Cuda::Details::SpacePoint&) {
  return !(bottom.radius > 150. && weight < 380.);
}

// Pointers to the device functions
static __device__ Acts::Cuda::TripletFilterConfig::seedWeightFunc_t
    seedWeightDeviceFunc = testSeedWeight;
static __device__ Acts::Cuda::TripletFilterConfig::singleSeedCutFunc_t
    singleSeedCutDeviceFunc = testSingleSeedCut;

Acts::Cuda::TripletFilterConfig testDeviceCuts() {
  // Create the filter configuration object.
  Acts::Cuda::TripletFilterConfig result;

  // Set up the function pointer variables on it to point at the functions
  // implemented in this source file.
  CUDA_CHECK(cudaMemcpyFromSymbol(
      &(result.seedWeight), ::seedWeightDeviceFunc,
      sizeof(Acts::Cuda::TripletFilterConfig::seedWeightFunc_t)));
  CUDA_CHECK(cudaMemcpyFromSymbol(
      &(result.singleSeedCut), ::singleSeedCutDeviceFunc,
      sizeof(Acts::Cuda::TripletFilterConfig::singleSeedCutFunc_t)));

  // Return the configured object.
  return result;
}
