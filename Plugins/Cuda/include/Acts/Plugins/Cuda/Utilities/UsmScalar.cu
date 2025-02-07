// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <iostream>
#include <memory>

#include "CudaUtils.cu"
#include "cuda.h"
#include "cuda_runtime.h"

namespace Acts {

template <typename var_t>
class UsmScalar {
 public:
  UsmScalar() {
    ACTS_CUDA_ERROR_CHECK(cudaMallocManaged((var_t**)&m_devPtr, sizeof(var_t)));
    cudaDeviceSynchronize();
  }

  UsmScalar(var_t scalar) {
    ACTS_CUDA_ERROR_CHECK(cudaMallocManaged((var_t**)&m_devPtr, sizeof(var_t)));
    cudaDeviceSynchronize();
    m_devPtr[0] = scalar;
  }

  UsmScalar(var_t* scalar) {
    ACTS_CUDA_ERROR_CHECK(cudaMallocManaged((var_t**)&m_devPtr, sizeof(var_t)));
    cudaDeviceSynchronize();
    m_devPtr[0] = *scalar;
  }

  UsmScalar(const var_t* scalar) {
    ACTS_CUDA_ERROR_CHECK(cudaMallocManaged((var_t**)&m_devPtr, sizeof(var_t)));
    cudaDeviceSynchronize();
    m_devPtr[0] = *scalar;
  }

  ~UsmScalar() {
    cudaDeviceSynchronize();
    ACTS_CUDA_ERROR_CHECK(cudaFree(m_devPtr));
  }

  var_t* get() { return m_devPtr; }
  void set(var_t scalar) { m_devPtr[0] = scalar; }

 private:
  var_t* m_devPtr;
};
}  // namespace Acts
