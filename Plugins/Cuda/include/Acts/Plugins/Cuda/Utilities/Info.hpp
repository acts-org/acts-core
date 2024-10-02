// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// System include(s).
#include <iosfwd>
#include <string>
#include <vector>

namespace Acts {
namespace Cuda {

/// Class providing information about the CUDA devices at runtime
///
/// Without exposing any CUDA dependencies publicly to the clients.
///
class Info {
 public:
  /// @name Declarations preventing any copies of the singleton object
  /// @{

  /// Explicitly delete the copy constructor
  Info(const Info&) = delete;
  /// Explicitly delete the move constructor
  Info(Info&&) = delete;

  /// Explicitly delete the copy assignment operator
  Info& operator=(const Info&) = delete;
  /// Explicitly delete the move assignment operator
  Info& operator=(Info&&) = delete;

  /// @}

  /// Singleton accessor function
  static Info& instance();

  /// Helper struct describing one available CUDA device
  struct Device {
    /// Identifier that CUDA knows this device by
    int id = -1;
    /// The name of this device
    std::string name;
    /// The maximal number of threads per block for this device
    int maxThreadsPerBlock = -1;
    /// Whether the device supports multiple kernel executions in parallel
    bool concurrentKernels = false;
    /// The total amount of (global) memory on the device
    std::size_t totalMemory = 0;
  };  // struct Device

  /// Get all the available CUDA devices
  const std::vector<Device>& devices() const;

 private:
  /// The constructor is private to implement the singleton behaviour
  Info();

  /// Information about all available devices
  std::vector<Device> m_devices;

};  // class Info

/// Print operator for @c Acts::Cuda::Info::Device
std::ostream& operator<<(std::ostream& out, const Info::Device& device);

}  // namespace Cuda
}  // namespace Acts
