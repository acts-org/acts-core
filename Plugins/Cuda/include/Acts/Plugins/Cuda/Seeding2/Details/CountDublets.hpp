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

/// Calculate summary values for the dublet search
///
/// After the dublet search is done, we need to know some information about how
/// many duplets were found exactly. As this information is necessary for the
/// scheduing of the subsequent steps of the execution on the GPU. This function
/// is used to collect this information
///
/// @param maxBlockSize The maximum block size to use on the GPU
/// @param nMiddleSP The number of middle spacepoints for which the dublet
///        reconstruction was run
/// @param middleBottomCounts The output from the
///        @c Acts::Cuda::Details::findDublets(...) function with the same name
/// @param middleTopCounts The output from the
///        @c Acts::Cuda::Details::findDublets(...) function with the same name
/// @return An object holding all the summary statistics necessary for the
///         subsequent steps of GPU execution
///
DubletCounts countDublets(std::size_t maxBlockSize, std::size_t nMiddleSP,
                          const device_array<unsigned int>& middleBottomCounts,
                          const device_array<unsigned int>& middleTopCounts);

}  // namespace Details
}  // namespace Cuda
}  // namespace Acts
