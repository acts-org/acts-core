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

// System include(s).
#include <cstddef>
#include <memory>

namespace Acts {
namespace Cuda {

/// Namespace holding some implementation detail types that should not be used
/// directly in client code.
namespace Details {

/// Class performing the deletion of a CUDA device memory array
class DeviceArrayDeleter {
 public:
  /// Operator performing the deletion of the memory
  void operator()(void* ptr);

};  // class DeviceArrayDeleter

/// Class performing the deletion of pinned host memory
class HostArrayDeleter {
 public:
  /// Operator performing the deletion of the memory
  void operator()(void* ptr);

};  // class HostArrayDeleter

}  // namespace Details

/// Convenience type for using primitive variable arrays on a CUDA device
template <typename T>
using device_array = std::unique_ptr<T, Details::DeviceArrayDeleter>;

/// Function creating a primitive array in CUDA device memory
template <typename T>
device_array<T> make_device_array(std::size_t size);

/// Convenience type for using primitive variable arrays on the host
template <typename T>
using host_array = std::unique_ptr<T, Details::HostArrayDeleter>;

/// Function creating a primitive array in the host's memory
template <typename T>
host_array<T> make_host_array(std::size_t size);

/// Copy one array from the host to the device
template <typename T>
void copyToDevice(device_array<T>& dev, const host_array<T>& host,
                  std::size_t arraySize);

/// Copy one array from the host to the device asynchronously
template <typename T>
void copyToDevice(device_array<T>& dev, const host_array<T>& host,
                  std::size_t arraySize, const StreamWrapper& stream);

/// Copy one array from the device to the host
template <typename T>
void copyToHost(host_array<T>& host, const device_array<T>& dev,
                std::size_t arraySize);

/// Copy one array from the device to the host asynchronously
template <typename T>
void copyToHost(host_array<T>& host, const device_array<T>& dev,
                std::size_t arraySize, const StreamWrapper& stream);

}  // namespace Cuda
}  // namespace Acts
