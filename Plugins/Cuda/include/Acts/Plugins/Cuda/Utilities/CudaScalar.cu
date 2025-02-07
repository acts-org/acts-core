// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Cuda/Utilities/CpuScalar.hpp"

#include <iostream>
#include <memory>

#include "CudaUtils.cu"
#include "cuda.h"
#include "cuda_runtime.h"

namespace Acts {

template <typename var_t>
class CpuScalar;

template <typename var_t>
class CudaScalar {
 public:
  CudaScalar() {
    ACTS_CUDA_ERROR_CHECK(cudaMalloc((var_t**)&m_devPtr, sizeof(var_t)));
  }

  CudaScalar(var_t* scalar) {
    ACTS_CUDA_ERROR_CHECK(cudaMalloc((var_t**)&m_devPtr, sizeof(var_t)));
    ACTS_CUDA_ERROR_CHECK(
        cudaMemcpy(m_devPtr, scalar, sizeof(var_t), cudaMemcpyHostToDevice));
  }

  CudaScalar(const var_t* scalar) {
    ACTS_CUDA_ERROR_CHECK(cudaMalloc((var_t**)&m_devPtr, sizeof(var_t)));
    ACTS_CUDA_ERROR_CHECK(
        cudaMemcpy(m_devPtr, scalar, sizeof(var_t), cudaMemcpyHostToDevice));
  }

  ~CudaScalar() { ACTS_CUDA_ERROR_CHECK(cudaFree(m_devPtr)); }

  var_t* get() { return m_devPtr; }

  void zeros() { cudaMemset(m_devPtr, 0, sizeof(var_t)); }

 private:
  var_t* m_devPtr;
};
}  // namespace Acts
