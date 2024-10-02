// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// CUDA2 plugin include(s).
#include "Acts/Plugins/Cuda/Seeding2/Details/Types.hpp"
#include "Acts/Plugins/Cuda/Utilities/Arrays.hpp"

// System include(s).
#include <cstddef>

namespace Acts {
namespace Cuda {
namespace Details {

/// Find all viable middle-bottom and middle-top dublets
///
/// This function is run as the first step in the seed finding, looking for
/// viable middle-bottom and middle-top spacepoint pairs for the subsequent
/// steps of the code.
///
/// Note that @c middleBottomCounts and @c middleTopCounts have type
/// "unsigned int" instead of "std::size_t", because the GPU code needs to
/// execute atomic operations on these arrays. And CUDA does not define such
/// operations on std::size_t (i.e. unsigned long).
///
/// @param[in] maxBlockSize The maximum block size to use on the GPU
/// @param[in] nBottomSPs The number of bottom spacepoints in @c bottomSPs
/// @param[in] bottomSPs Properties of all of the bottom spacepoints
/// @param[in] nMiddleSPs The number of middle spacepoints in @c middleSPs
/// @param[in] middleSPs Properties of all of the middle spacepoints
/// @param[in] nTopSPs The number of top spacepoints in @c topSPs
/// @param[in] topSPs Properties of all of the top spacepoints
/// @param[in] deltaRMin Configuration parameter from @c Acts::SeedFinderConfig
/// @param[in] deltaRMax Configuration parameter from @c Acts::SeedFinderConfig
/// @param[in] cotThetaMax Configuration parameter from
///            @c Acts::SeedFinderConfig
/// @param[in] collisionRegionMin Configuration parameter from
///            @c Acts::SeedFinderConfig
/// @param[in] collisionRegionMax Configuration parameter from
///            @c Acts::SeedFinderConfig
/// @param[out] middleBottomCounts 1-D array of the number of middle-bottom
///             dublets found for each middle spacepoint
/// @param[out] middleBottomDublets 2-D matrix of size
///             @c nMiddleSPs x @c nBottomSPs, holding the bottom spacepoint
///             indices for the identified middle-bottom dublets
/// @param[out] middleTopCounts 1-D array of the number of middle-top dublets
///             found for each middle spacepoint
/// @param[out] middleTopDublets 2-D matrix of size
///             @c nMiddleSPs x @c nTopSPs, holding the top spacepoint
///             indices for the identified middle-top dublets
///
void findDublets(std::size_t maxBlockSize, std::size_t nBottomSPs,
                 const device_array<SpacePoint>& bottomSPs,
                 std::size_t nMiddleSPs,
                 const device_array<SpacePoint>& middleSPs, std::size_t nTopSPs,
                 const device_array<SpacePoint>& topSPs, float deltaRMin,
                 float deltaRMax, float cotThetaMax, float collisionRegionMin,
                 float collisionRegionMax,
                 device_array<unsigned int>& middleBottomCounts,
                 device_array<std::size_t>& middleBottomDublets,
                 device_array<unsigned int>& middleTopCounts,
                 device_array<std::size_t>& middleTopDublets);

}  // namespace Details
}  // namespace Cuda
}  // namespace Acts
