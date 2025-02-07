// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Cuda/Utilities/CpuVector.hpp"

#include <iostream>
#include <memory>

#include "CudaUtils.cu"
#include "cuda.h"
#include "cuda_runtime.h"

namespace Acts {

template <typename var_t>
class CpuVector;

template <typename var_t>
class CudaVector {
 public:
  CudaVector() = delete;
  CudaVector(std::size_t size) {
    m_size = size;
    ACTS_CUDA_ERROR_CHECK(
        cudaMalloc((var_t**)&m_devPtr, m_size * sizeof(var_t)));
  }

  CudaVector(std::size_t size, var_t* vector) {
    m_size = size;
    ACTS_CUDA_ERROR_CHECK(
        cudaMalloc((var_t**)&m_devPtr, m_size * sizeof(var_t)));
    copyH2D(vector, m_size, 0);
  }

  CudaVector(std::size_t size, var_t* vector, std::size_t len,
             std::size_t offset) {
    m_size = size;
    ACTS_CUDA_ERROR_CHECK(
        cudaMalloc((var_t**)&m_devPtr, m_size * sizeof(var_t)));
    copyH2D(vector, len, offset);
  }

  ~CudaVector() {
    if (m_devPtr)
      cudaFree(m_devPtr);
  }

  var_t* get(std::size_t offset = 0) { return m_devPtr + offset; }

  void copyH2D(var_t* vector, std::size_t len, std::size_t offset) {
    ACTS_CUDA_ERROR_CHECK(cudaMemcpy(m_devPtr + offset, vector,
                                     len * sizeof(var_t),
                                     cudaMemcpyHostToDevice));
  }
  void copyH2D(var_t* vector, std::size_t len, std::size_t offset,
               cudaStream_t* stream) {
    ACTS_CUDA_ERROR_CHECK(cudaMemcpyAsync(m_devPtr + offset, vector,
                                          len * sizeof(var_t),
                                          cudaMemcpyHostToDevice, *stream));
  }

  void zeros() {
    ACTS_CUDA_ERROR_CHECK(cudaMemset(m_devPtr, 0, m_size * sizeof(var_t)));
  }

 private:
  var_t* m_devPtr = nullptr;
  std::size_t m_size;
};
}  // namespace Acts
