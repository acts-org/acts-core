// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// CUDA plugin include(s).
#include "Acts/Plugins/Cuda/Seeding2/Details/Types.hpp"
#include "Acts/Plugins/Cuda/Utilities/Arrays.hpp"
#include "Acts/Plugins/Cuda/Utilities/Info.hpp"

// System include(s).
#include <cstddef>
#include <vector>

namespace Acts {

// Forward declaration(s).
struct SeedFilterConfig;

namespace Cuda {

// Forward declaration(s).
struct TripletFilterConfig;

namespace Details {

/// Find all viable triplets from the provided spacepoint dublets
///
/// This function is used to find a "loosely selected" set of seed candidates
/// that still need to be filtered through
/// @c Acts::SeedFilter::filterSeeds_1SpFixed before returning it to the user.
///
/// @param[in] device Properties of the device that the code will be running on
/// @param[in] maxBlockSize The maximum block size to use on the GPU
/// @param[in] dubletCounts The output object from
///            @c Acts::Cuda::Details::countDublets
/// @param[in] seedConfig Configuration parameters for the triplet
///            finding/filtering
/// @param[in] filterConfig User provided settings (code...) for the triplet
///            filtering
/// @param[in] nBottomSPs The number of bottom spacepoints in @c bottomSPs
/// @param[in] bottomSPs Properties of all of the bottom spacepoints
/// @param[in] nMiddleSPs The number of middle spacepoints in @c middleSPs
/// @param[in] middleSPs Properties of all of the middle spacepoints
/// @param[in] nTopSPs The number of top spacepoints in @c topSPs
/// @param[in] topSPs Properties of all of the top spacepoints
/// @param[in] middleBottomCounts 1-D array of the number of middle-bottom
///            dublets found for each middle spacepoint
/// @param[in] middleBottomDublets 2-D matrix of size
///            @c nMiddleSPs x @c nBottomSPs, holding the bottom spacepoint
///            indices for the identified middle-bottom dublets
/// @param[in] middleTopCounts 1-D array of the number of middle-top dublets
///            found for each middle spacepoint
/// @param[in] middleTopDublets 2-D matrix of size
///            @c nMiddleSPs x @c nTopSPs, holding the top spacepoint
///            indices for the identified middle-top dublets
/// @param[in] maxScatteringAngle2 Configuration parameter from
///            @c Acts::SeedFinderConfig
/// @param[in] sigmaScattering Configuration parameter from
///            @c Acts::SeedFinderConfig
/// @param[in] minHelixDiameter2 Configuration parameter from
///            @c Acts::SeedFinderConfig
/// @param[in] pT2perRadius Configuration parameter from
///            @c Acts::SeedFinderConfig
/// @param[in] impactMax Configuration parameter from @c Acts::SeedFinderConfig
/// @return A 2-D structure holding the parameters of the identified triplets
///         for each middle spacepoint
///
std::vector<std::vector<Triplet> > findTriplets(
    const Info::Device& device, std::size_t maxBlockSize,
    const DubletCounts& dubletCounts, const SeedFilterConfig& seedConfig,
    const TripletFilterConfig& filterConfig, std::size_t nBottomSPs,
    const device_array<SpacePoint>& bottomSPs, std::size_t nMiddleSPs,
    const device_array<SpacePoint>& middleSPs, std::size_t nTopSPs,
    const device_array<SpacePoint>& topSPs,
    const device_array<unsigned int>& middleBottomCounts,
    const device_array<std::size_t>& middleBottomDublets,
    const device_array<unsigned int>& middleTopCounts,
    const device_array<std::size_t>& middleTopDublets,
    float maxScatteringAngle2, float sigmaScattering, float minHelixDiameter2,
    float pT2perRadius, float impactMax);

}  // namespace Details
}  // namespace Cuda
}  // namespace Acts
