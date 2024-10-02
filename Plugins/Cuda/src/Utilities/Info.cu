// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// CUDA plugin include(s).
#include "Acts/Plugins/Cuda/Utilities/Info.hpp"

#include "ErrorCheck.cuh"

// System include(s).
#include <cmath>
#include <iostream>

namespace Acts {
namespace Cuda {

Info& Info::instance() {
  static Info info;
  return info;
}

const std::vector<Info::Device>& Info::devices() const {
  return m_devices;
}

Info::Info() {
  // Collect all information about all the available devices on
  // construction. Note that we explicitly ignore the return value of the call,
  // in case the code is executed without any available CUDA devices.
  int nDevices = 0;
  static_cast<void>(cudaGetDeviceCount(&nDevices));

  for (int i = 0; i < nDevices; ++i) {
    // Retrieve all properties of this device.
    cudaDeviceProp properties;
    ACTS_CUDA_ERROR_CHECK(cudaGetDeviceProperties(&properties, i));

    // Create an @c Acts::Cuda::Info::Device object from the information.
    m_devices.push_back({i, properties.name, properties.maxThreadsPerBlock,
                         static_cast<bool>(properties.concurrentKernels),
                         properties.totalGlobalMem});
  }
}

std::ostream& operator<<(std::ostream& out, const Info::Device& device) {
  out << " /-- Device ID " << device.id << " " << std::string(31, '-') << "\\"
      << std::endl;
  out << " | Name: " << device.name
      << std::string(
             (39 > device.name.length() ? 39 - device.name.length() : 0), ' ')
      << "|" << std::endl;
  const std::size_t threadDigits =
      static_cast<std::size_t>(std::log10(device.maxThreadsPerBlock)) + 1;
  out << " | Max. threads per block: " << device.maxThreadsPerBlock
      << std::string((21 > threadDigits ? 21 - threadDigits : 0), ' ') << "|"
      << std::endl;
  out << " | Concurrent kernels: "
      << (device.concurrentKernels ? "true " : "false") << std::string(20, ' ')
      << "|" << std::endl;
  static constexpr double MEGABYTES = 1.0 / (1024 * 1024);
  const double totalMem = device.totalMemory * MEGABYTES;
  const std::size_t memDigits =
      static_cast<std::size_t>(std::log10(totalMem)) + 1;
  out << " | Total memory: " << totalMem << " MB"
      << std::string((25 > memDigits ? 25 - memDigits : 0), ' ') << "|"
      << std::endl;
  out << " \\" << std::string(46, '-') << "/";

  return out;
}

}  // namespace Cuda
}  // namespace Acts
