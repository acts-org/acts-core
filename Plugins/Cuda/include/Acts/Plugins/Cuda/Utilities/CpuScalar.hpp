// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
