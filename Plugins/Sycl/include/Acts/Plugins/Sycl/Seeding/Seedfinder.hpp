// This file is part of the Acts project.
//
// Copyright (C) 2020-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Acts include(s).
#include "Acts/Seeding/SeedFilter.hpp"
#include "Acts/Seeding/SeedfinderConfig.hpp"

// SYCL plugin include(s).
#include "Acts/Plugins/Sycl/Seeding/DeviceExperimentCuts.hpp"
#include "Acts/Plugins/Sycl/Seeding/detail/Types.hpp"
#include "Acts/Plugins/Sycl/Utilities/QueueWrapper.hpp"

// VecMem include(s).
#include "vecmem/memory/memory_resource.hpp"

namespace Acts::Sycl {

template <typename external_spacepoint_t>
class Seedfinder {
 public:
  Seedfinder(
      Acts::SeedfinderConfig<external_spacepoint_t> config,
      const Acts::Sycl::DeviceExperimentCuts& cuts,
      Acts::Sycl::QueueWrapper wrappedQueue,
      vecmem::memory_resource& resource,
      vecmem::memory_resource* device_resource = nullptr);

  ~Seedfinder() = default;
  Seedfinder() = delete;
  Seedfinder(const Seedfinder<external_spacepoint_t>&) = delete;
  Seedfinder<external_spacepoint_t>& operator=(
      const Seedfinder<external_spacepoint_t>&) = delete;

  /// Create all seeds from the space points in the three iterators.
  /// Can be used to parallelize the seed creation
  /// @param bottomSPs group of space points to be used as innermost SP in a seed.
  /// @param middleSPs group of space points to be used as middle SP in a seed.
  /// @param topSPs group of space points to be used as outermost SP in a seed.
  /// Ranges must return pointers.
  /// Ranges must be separate objects for each parallel call.
  /// @return vector in which all found seeds for this group are stored.
  template <typename sp_range_t>
  std::vector<Seed<external_spacepoint_t>> createSeedsForGroup(
      sp_range_t bottomSPs, sp_range_t middleSPs, sp_range_t topSPs) const;

 private:
  Acts::SeedfinderConfig<external_spacepoint_t> m_config;

  /// Experiment specific cuts
  Acts::Sycl::DeviceExperimentCuts m_deviceCuts;

  /// Configuration object for the device side.
  Acts::Sycl::detail::DeviceSeedfinderConfig m_deviceConfig;

  /// Wrapper around a SYCL queue object.
  QueueWrapper m_wrappedQueue;

  /// host/shared memory resource to use in the seed-finder
  vecmem::memory_resource* m_resource;

  /// Device memory resource for use
  vecmem::memory_resource* m_device_resource;
};

}  // namespace Acts::Sycl

// Include the template implementation.
#include "Acts/Plugins/Sycl/Seeding/Seedfinder.ipp"
