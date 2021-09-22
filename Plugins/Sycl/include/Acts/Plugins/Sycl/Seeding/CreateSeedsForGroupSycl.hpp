// This file is part of the Acts project.
//
// Copyright (C) 2020-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// System include(s)
#include <vector>

// VecMem include(s).
#include "vecmem/containers/vector.hpp"
#include "vecmem/memory/memory_resource.hpp"
#include "vecmem/containers/jagged_vector.hpp"

// SYCL plugin include(s)
#include "Acts/Plugins/Sycl/Seeding/DeviceExperimentCuts.hpp"
#include "Acts/Plugins/Sycl/Seeding/detail/Types.hpp"
#include "Acts/Plugins/Sycl/Utilities/QueueWrapper.hpp"


namespace Acts::Sycl {

/// @brief Seedfinding algorithm implemented in SYCL.
///
/// @param[in] wrappedQueue is a wrapper object of the SYCL queue
/// @param[in] seedfinderConfig includes the required configuration
/// parameters for the algorithm
/// @param[in] deviceCuts is an experiment specific object with customizable
/// seed weight altering and seed cutting member functions
/// @param[in] bottomSPs an array of simplified internal space
///                      point structures of bottom space points
/// @param[in] middleSPs an array of simplified internal space
///                      point structures of middle space points
/// @param[in] topSPs an array of simplified internal space
///                      point structures of top space points
/// @param[out] seeds holds of the generated seed indices and weight
void createSeedsForGroupSycl(
    QueueWrapper wrappedQueue,
    vecmem::memory_resource& resource,
    vecmem::memory_resource* device_resource,
    const detail::DeviceSeedfinderConfig& seedfinderConfig,
    const DeviceExperimentCuts& deviceCuts,
    vecmem::vector<detail::DeviceSpacePoint>& bottomSPs,
    vecmem::vector<detail::DeviceSpacePoint>& middleSPs,
    vecmem::vector<detail::DeviceSpacePoint>& topSPs,
    std::vector<std::vector<detail::SeedData>>& seeds);
}  // namespace Acts::Sycl
