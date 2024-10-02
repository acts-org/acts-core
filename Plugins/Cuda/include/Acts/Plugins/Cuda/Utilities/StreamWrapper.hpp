// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// CUDA plugin include(s).
#include "Acts/Plugins/Cuda/Utilities/Info.hpp"

namespace Acts {
namespace Cuda {

/// Helper class for passing around @c cudaStream_t objects (pointers)
///
/// In order to be able to create user interfaces that return/receive CUDA
/// streams, while not exposing the users of those interfaces to the CUDA
/// Runtime API, this class helps us hiding the concrete CUDA types from our
/// interfaces.
///
class StreamWrapper {
/// Declare the @c Acts::Cuda::getStreamFrom function a frient of the class
///
/// Note that it's not practical to put that function into the
/// @c Acts::Cuda::details namespace, because then we would be forced to
/// forward declare it in this header.
#ifdef __CUDACC__
  friend cudaStream_t getStreamFrom(const StreamWrapper&);
#endif  // __CUDACC__

 public:
  /// Constructor with the stream to be wrapped
  StreamWrapper(void* stream, bool ownsStream = true);
  /// Move constructor
  StreamWrapper(StreamWrapper&& parent);
  /// Disabled copy constructor
  StreamWrapper(const StreamWrapper&) = delete;
  /// Destructor
  ~StreamWrapper();

  /// Move assignment operator
  StreamWrapper& operator=(StreamWrapper&& rhs);
  /// Disabled copy assignment operator
  StreamWrapper& operator=(const StreamWrapper&) = delete;

  /// Wait for all scheduled operations to finish in the stream
  void synchronize() const;

 private:
  /// Type erased pointer, managed by this wrapper class
  void* m_stream;
  /// Flag showing whether the object owns the stream that it wraps
  bool m_ownsStream;

};  // class StreamWrapper

/// Create a stream for a particular CUDA device
StreamWrapper createStreamFor(const Acts::Cuda::Info::Device& device);

}  // namespace Cuda
}  // namespace Acts
