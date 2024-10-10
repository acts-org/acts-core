// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// System include(s).
#include <cstddef>
#include <vector>

namespace Acts {
namespace Cuda {

/// Singleton class used for allocating memory on CUDA device(s)
///
/// In order to avoid calling @c cudaMalloc(...) and @c cudaFree(...) too many
/// times in the code (which can turn out to be pretty slow), device memory
/// is allocated using this singleton memory manager for the
/// @c Acts::Cuda::device_array arrays.
///
/// It is implemented in a **very** simple way. It allocates a big blob of
/// memory, and then hands out pointers from this blob to anyone that asks for
/// device memory.
///
/// The class doesn't handle memory returns in any sophisticated way. It assumes
/// that any calculation will need all allocated memory until the end of that
/// calculation. At which point all of that memory gets re-purpused in one call.
///
/// The code is not thread safe currently in any shape or form. But there should
/// be ways of making it at least "thread friendly" later on.
///
class MemoryManager {
 public:
  /// Destructor, freeing up all allocated memory
  ~MemoryManager();

  /// @name Declarations preventing any copies of the singleton object
  /// @{

  /// Disallow copy construction
  MemoryManager(const MemoryManager&) = delete;
  /// Disallow move construction
  MemoryManager(MemoryManager&&) = delete;

  /// Disallow copy assignment
  MemoryManager& operator=(const MemoryManager&) = delete;
  /// Disallow move assignment
  MemoryManager& operator=(MemoryManager&&) = delete;

  /// @}

  /// @name Functions that the users of Acts may be interacting with
  /// @{

  /// Singleton object accessor
  static MemoryManager& instance();

  /// Set the amount of memory to use on a particular device
  void setMemorySize(std::size_t sizeInBytes, int device = -1);

  /// @}

  /// @name Functions used internally by the Acts code
  /// @{

  /// Get the amount of memory still available on a specific device
  std::size_t availableMemory(int device = -1) const;

  /// Get a pointer to an available memory block on the device
  void* allocate(std::size_t sizeInBytes, int device = -1);

  /// Reset all allocations
  void reset(int device = -1);

  /// @}

 private:
  /// Hide the constructor of the class
  MemoryManager();

  /// Struct describing the state of the memory allocation on a particular
  /// device
  struct DeviceMemory {
    /// The amount of memory allocated on the CUDA device
    std::size_t m_size = 0;
    /// Pointer to the beginning of the memory allocation
    char* m_ptr = nullptr;
    /// Pointer to the next available memory block in the "current round"
    char* m_nextAllocation = nullptr;
    /// The maximum amount of memory used at a time during the job
    std::ptrdiff_t m_maxUsage = 0;
  };

  /// Object holding information about memory allocations on all devices
  std::vector<DeviceMemory> m_memory;

};  // class MemoryManager

}  // namespace Cuda
}  // namespace Acts
