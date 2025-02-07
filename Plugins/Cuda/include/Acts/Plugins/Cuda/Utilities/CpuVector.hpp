// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Cuda/Utilities/CudaVector.cu"

#include <cstring>

namespace Acts {

template <typename var_t>
class CudaVector;

template <typename var_t>
class CpuVector {
 public:
  CpuVector() = delete;
  CpuVector(std::size_t size, bool pinned = false) {
    m_size = size;
    m_pinned = pinned;
    if (!pinned) {
      m_hostPtr = new var_t[m_size];
    } else if (pinned) {
      cudaMallocHost(&m_hostPtr, m_size * sizeof(var_t));
    }
  }

  CpuVector(std::size_t size, CudaVector<var_t>* cuVec, bool pinned = false) {
    m_size = size;
    m_pinned = pinned;
    if (!pinned) {
      m_hostPtr = new var_t[m_size];
    } else if (pinned) {
      cudaMallocHost(&m_hostPtr, m_size * sizeof(var_t));
    }
    cudaMemcpy(m_hostPtr, cuVec->get(), m_size * sizeof(var_t),
               cudaMemcpyDeviceToHost);
  }

  ~CpuVector() {
    if (!m_pinned) {
      delete m_hostPtr;
    } else if (m_pinned && m_hostPtr) {
      cudaFreeHost(m_hostPtr);
    }
  }

  var_t* get(std::size_t offset = 0) { return m_hostPtr + offset; }

  void set(std::size_t offset, var_t val) { m_hostPtr[offset] = val; }

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
  std::size_t m_size;
  bool m_pinned;
};

}  // namespace Acts
