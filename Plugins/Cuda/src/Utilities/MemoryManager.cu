// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// CUDA plugin include(s).
#include "Acts/Plugins/Cuda/Utilities/MemoryManager.hpp"

#include "ErrorCheck.cuh"

// CUDA include(s).
#include <cuda_runtime.h>

// System include(s).
#include <cmath>
#include <stdexcept>

namespace Acts {
namespace Cuda {

MemoryManager::~MemoryManager() {
  // Free all the allocated memory.
  for (DeviceMemory& mem : m_memory) {
    if (mem.m_ptr == nullptr) {
      continue;
    }
    ACTS_CUDA_ERROR_CHECK(cudaFree(mem.m_ptr));
  }
}

MemoryManager& MemoryManager::instance() {
  static MemoryManager mm;
  return mm;
}

void MemoryManager::setMemorySize(std::size_t sizeInBytes, int device) {
  // If the user didn't ask for a specific device, use the one currently used by
  // CUDA.
  if (device == -1) {
    ACTS_CUDA_ERROR_CHECK(cudaGetDevice(&device));
  }

  // Make sure that the internal storage variable is large enough.
  if (static_cast<std::size_t>(device) >= m_memory.size()) {
    m_memory.resize(device + 1);
  }

  // Get the object responsible for this device.
  DeviceMemory& mem = m_memory[device];

  // De-allocate any previously allocated memory.
  if (mem.m_ptr) {
    ACTS_CUDA_ERROR_CHECK(cudaFree(mem.m_ptr));
  }

  // Allocate the newly requested amount.
  ACTS_CUDA_ERROR_CHECK(cudaSetDevice(device));
  ACTS_CUDA_ERROR_CHECK(cudaMalloc(&(mem.m_ptr), sizeInBytes));

  // Set up the internal state of the object correctly.
  mem.m_size = sizeInBytes;
  mem.m_nextAllocation = mem.m_ptr;
  return;
}

std::size_t MemoryManager::availableMemory(int device) const {
  // If the user didn't ask for a specific device, use the one currently used by
  // CUDA.
  if (device == -1) {
    ACTS_CUDA_ERROR_CHECK(cudaGetDevice(&device));
  }

  // Make sure that memory was allocated on the requested device.
  if (m_memory.size() <= static_cast<std::size_t>(device)) {
    throw std::bad_alloc();
  }
  const DeviceMemory& mem = m_memory[device];

  // Return the requested information.
  return (mem.m_size - (mem.m_nextAllocation - mem.m_ptr));
}

void* MemoryManager::allocate(std::size_t sizeInBytes, int device) {
  // If the user didn't ask for a specific device, use the one currently used by
  // CUDA.
  if (device == -1) {
    ACTS_CUDA_ERROR_CHECK(cudaGetDevice(&device));
  }

  // Make sure that memory was allocated on the requested device.
  if (m_memory.size() <= static_cast<std::size_t>(device)) {
    throw std::bad_alloc();
  }
  DeviceMemory& mem = m_memory[device];

  // We already know what we want to return...
  void* result = mem.m_nextAllocation;

  // Make sure that all addresses given out are 8-byte aligned.
  static constexpr std::size_t ALIGN_SIZE = 8;
  const std::size_t misalignment = sizeInBytes % ALIGN_SIZE;
  const std::size_t padding =
      ((misalignment != 0) ? (ALIGN_SIZE - misalignment) : 0);

  // Increment the internal pointer.
  mem.m_nextAllocation += sizeInBytes + padding;
  // And make sure that we didn't run out of memory.
  if (mem.m_nextAllocation - mem.m_ptr >= mem.m_size) {
    throw std::bad_alloc();
  }

  // Apparently everything is okay.
  return result;
}

void MemoryManager::reset(int device) {
  // If the user didn't ask for a specific device, use the one currently used by
  // CUDA.
  if (device == -1) {
    ACTS_CUDA_ERROR_CHECK(cudaGetDevice(&device));
  }

  // Make sure that memory was allocated on the requested device.
  if (m_memory.size() <= static_cast<std::size_t>(device)) {
    throw std::bad_alloc();
  }
  DeviceMemory& mem = m_memory[device];

  // Note down how much memory was used in total until the reset.
  mem.m_maxUsage = std::max(mem.m_maxUsage, mem.m_nextAllocation - mem.m_ptr);
  // Return the internal pointer to its startout location.
  mem.m_nextAllocation = mem.m_ptr;
  return;
}

MemoryManager::MemoryManager() {
  // Allocate 1500 MBs of memory as a start on the default device.
  setMemorySize(1500 * 1024l * 1024l);
}

}  // namespace Cuda
}  // namespace Acts
