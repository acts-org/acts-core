// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Cuda/Utilities/CpuMatrix.hpp"

#include <iostream>
#include <memory>

#include "CudaUtils.cu"
#include "cuda.h"
#include "cuda_runtime.h"

namespace Acts {

template <typename var_t>
class CpuMatrix;

template <typename var_t>
class CudaMatrix {
 public:
  CudaMatrix() = delete;
  CudaMatrix(std::size_t nRows, std::size_t nCols) {
    m_setSize(nRows, nCols);
    ACTS_CUDA_ERROR_CHECK(
        cudaMalloc((var_t**)&m_devPtr, m_nRows * m_nCols * sizeof(var_t)));
  }

  CudaMatrix(std::size_t nRows, std::size_t nCols, var_t* mat) {
    m_setSize(nRows, nCols);
    ACTS_CUDA_ERROR_CHECK(
        cudaMalloc((var_t**)&m_devPtr, m_nRows * m_nCols * sizeof(var_t)));
    copyH2D(mat, m_size, 0);
  }

  CudaMatrix(std::size_t nRows, std::size_t nCols, CpuMatrix<var_t>* mat) {
    m_setSize(nRows, nCols);
    ACTS_CUDA_ERROR_CHECK(
        cudaMalloc((var_t**)&m_devPtr, m_nRows * m_nCols * sizeof(var_t)));
    copyH2D(mat->get(0, 0), m_size, 0);
  }

  CudaMatrix(std::size_t nRows, std::size_t nCols, var_t* mat, std::size_t len,
             std::size_t offset) {
    m_setSize(nRows, nCols);
    ACTS_CUDA_ERROR_CHECK(
        cudaMalloc((var_t**)&m_devPtr, m_nRows * m_nCols * sizeof(var_t)));
    copyH2D(mat, len, offset);
  }

  CudaMatrix(std::size_t nRows, std::size_t nCols, CpuMatrix<var_t>* mat,
             std::size_t len, std::size_t offset) {
    m_setSize(nRows, nCols);
    ACTS_CUDA_ERROR_CHECK(
        cudaMalloc((var_t**)&m_devPtr, m_nRows * m_nCols * sizeof(var_t)));
    copyH2D(mat->get(0, 0), len, offset);
  }

  ~CudaMatrix() {
    if (m_devPtr)
      cudaFree(m_devPtr);
  }

  var_t* get(std::size_t row = 0, std::size_t col = 0) {
    int offset = row + col * m_nRows;
    return m_devPtr + offset;
  }

  void copyH2D(var_t* matrix, std::size_t len, std::size_t offset = 0) {
    ACTS_CUDA_ERROR_CHECK(cudaMemcpy(m_devPtr + offset, matrix,
                                     len * sizeof(var_t),
                                     cudaMemcpyHostToDevice));
  }

  void copyH2D(const var_t* matrix, std::size_t len, std::size_t offset = 0) {
    ACTS_CUDA_ERROR_CHECK(cudaMemcpy(m_devPtr + offset, matrix,
                                     len * sizeof(var_t),
                                     cudaMemcpyHostToDevice));
  }

  void zeros() {
    ACTS_CUDA_ERROR_CHECK(cudaMemset(m_devPtr, 0, m_size * sizeof(var_t)));
  }

 private:
  var_t* m_devPtr = nullptr;
  std::size_t m_nCols;
  std::size_t m_nRows;
  std::size_t m_size;

  void m_setSize(std::size_t row, std::size_t col) {
    m_nRows = row;
    m_nCols = col;
    m_size = m_nRows * m_nCols;
  }
};

}  // namespace Acts
