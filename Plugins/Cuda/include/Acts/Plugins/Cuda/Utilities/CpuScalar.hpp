// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Cuda/Utilities/CudaScalar.cu"

namespace Acts {

template <typename var_t>
class CudaScalar;

template <typename var_t>
class CpuScalar {
 public:
  CpuScalar(bool pinned = false) {
    m_pinned = pinned;
    if (!pinned) {
      m_hostPtr = new var_t[1];
    } else if (pinned) {
      cudaMallocHost(&m_hostPtr, sizeof(var_t));
    }
  }

  CpuScalar(CudaScalar<var_t>* cuScalar, bool pinned = false) {
    m_pinned = pinned;
    if (!pinned) {
      m_hostPtr = new var_t[1];
    } else if (pinned) {
      cudaMallocHost(&m_hostPtr, sizeof(var_t));
    }
    cudaMemcpy(m_hostPtr, cuScalar->get(), sizeof(var_t),
               cudaMemcpyDeviceToHost);
  }

  ~CpuScalar() {
    if (!m_pinned) {
      delete m_hostPtr;
    } else if (m_pinned && m_hostPtr) {
      cudaFreeHost(m_hostPtr);
    }
  }

  var_t* get() { return m_hostPtr; }

  void Set(var_t val) { m_hostPtr[0] = val; }

 private:
  var_t* m_hostPtr = nullptr;
  std::size_t m_size;
  bool m_pinned;
};

}  // namespace Acts
