// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

// CUDA plugin include(s).
#include "Acts/Plugins/Cuda/Utilities/StreamWrapper.hpp"

#include "ErrorCheck.cuh"
#include "StreamHandlers.cuh"

// CUDA include(s).
#include <cuda_runtime.h>

namespace Acts {
namespace Cuda {

StreamWrapper::StreamWrapper(void* stream, bool ownsStream)
    : m_stream(stream), m_ownsStream(ownsStream) {}

StreamWrapper::StreamWrapper(StreamWrapper&& parent)
    : m_stream(parent.m_stream), m_ownsStream(parent.m_ownsStream) {
  parent.m_stream = nullptr;
  parent.m_ownsStream = false;
}

StreamWrapper::~StreamWrapper() {
  // Destroy the stream, if we still hold it.
  if (m_stream && m_ownsStream) {
    ACTS_CUDA_ERROR_CHECK(cudaStreamDestroy(getStreamFrom(*this)));
  }
}

StreamWrapper& StreamWrapper::operator=(StreamWrapper&& rhs) {
  // Check whether anything needs to be done.
  if (this == &rhs) {
    return *this;
  }

  // Destroy the current stream, if we hold one.
  if (m_stream && m_ownsStream) {
    ACTS_CUDA_ERROR_CHECK(cudaStreamDestroy(getStreamFrom(*this)));
  }

  // Perform the move.
  m_stream = rhs.m_stream;
  m_ownsStream = rhs.m_ownsStream;
  rhs.m_stream = nullptr;
  rhs.m_ownsStream = false;

  // Return this object.
  return *this;
}

void StreamWrapper::synchronize() const {
  // Use CUDA to wait for all tasks to finish in the stream.
  ACTS_CUDA_ERROR_CHECK(cudaStreamSynchronize(getStreamFrom(*this)));
  return;
}

StreamWrapper createStreamFor(const Acts::Cuda::Info::Device& device) {
  // Create the stream for the selected device.
  ACTS_CUDA_ERROR_CHECK(cudaSetDevice(device.id));
  cudaStream_t stream = nullptr;
  ACTS_CUDA_ERROR_CHECK(
      cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking));

  // Return the new object.
  return StreamWrapper(stream);
}

}  // namespace Cuda
}  // namespace Acts
