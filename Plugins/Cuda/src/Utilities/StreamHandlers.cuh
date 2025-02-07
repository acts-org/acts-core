// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

// CUDA plugin include(s).
#include "Acts/Plugins/Cuda/Utilities/StreamWrapper.hpp"

// CUDA include(s).
#include <cuda_runtime.h>

namespace Acts {
namespace Cuda {

/// Get the @c cudaStream_t value out of an @c Acts::Cuda::StreamWrapper object
inline cudaStream_t getStreamFrom(const StreamWrapper& wrapper) {
  return static_cast<cudaStream_t>(wrapper.m_stream);
}

}  // namespace Cuda
}  // namespace Acts
