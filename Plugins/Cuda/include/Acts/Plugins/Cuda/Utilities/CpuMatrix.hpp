// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Cuda/Utilities/CudaMatrix.cu"

// column-major style Matrix Definition

namespace Acts {

template <typename var_t>
class CudaMatrix;

template <typename var_t>
class CpuMatrix {
 public:
  CpuMatrix() = delete;
  CpuMatrix(std::size_t nRows, std::size_t nCols, bool pinned = false) {
    m_setSize(nRows, nCols);
    m_pinned = pinned;
    if (!pinned) {
      m_hostPtr = new var_t[m_size];
    } else if (pinned) {
      cudaMallocHost(&m_hostPtr, m_size * sizeof(var_t));
    }
  }

  CpuMatrix(std::size_t nRows, std::size_t nCols, CudaMatrix<var_t>* cuMat,
            bool pinned = false) {
    m_setSize(nRows, nCols);
    m_pinned = pinned;
    if (!pinned) {
      m_hostPtr = new var_t[m_size];
    } else if (pinned) {
      cudaMallocHost(&m_hostPtr, m_nRows * m_nCols * sizeof(var_t));
    }
    cudaMemcpy(m_hostPtr, cuMat->get(0, 0), m_size * sizeof(var_t),
               cudaMemcpyDeviceToHost);
  }

  ~CpuMatrix() {
    if (!m_pinned) {
      delete m_hostPtr;
    } else if (m_pinned && m_hostPtr) {
      cudaFreeHost(m_hostPtr);
    }
  }

  var_t* get(std::size_t row = 0, std::size_t col = 0) {
    std::size_t offset = row + col * m_nRows;
    return m_hostPtr + offset;
  }

  void set(std::size_t row, std::size_t col, var_t val) {
    std::size_t offset = row + col * m_nRows;
    m_hostPtr[offset] = val;
  }

  void copyD2H(var_t* devPtr, std::size_t len, std::size_t offset) {
    cudaMemcpy(m_hostPtr + offset, devPtr, len * sizeof(var_t),
               cudaMemcpyDeviceToHost);
  }

  void copyD2H(var_t* devPtr, std::size_t len, std::size_t offset,
               cudaStream_t* stream) {
    cudaMemcpyAsync(m_hostPtr + offset, devPtr, len * sizeof(var_t),
                    cudaMemcpyDeviceToHost, *stream);
  }

  void zeros() { memset(m_hostPtr, 0, m_size * sizeof(var_t)); }

 private:
  var_t* m_hostPtr = nullptr;
  std::size_t m_nCols;
  std::size_t m_nRows;
  std::size_t m_size;
  bool m_pinned;

  void m_setSize(std::size_t row, std::size_t col) {
    m_nRows = row;
    m_nCols = col;
    m_size = m_nRows * m_nCols;
  }
};

}  // namespace Acts
