// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
