// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Surfaces/Surface.hpp"
#include "Acts/TrackFitting/GsfOptions.hpp"

namespace Acts {

/// Very simple mixture reduction method: Just removes the components with the
/// smallest weight until the required number of components is reached
/// @param cmpCache the component collection
/// @param maxCmpsAfterMerge the number of components we want to reach
/// @param surface the surface type on which the components are (unused here)
void reduceMixtureLargestWeights(std::vector<Acts::GsfComponent> &cmpCache,
                                 std::size_t maxCmpsAfterMerge,
                                 const Surface &surface);

/// Greedy component reduction algorithm. Reduces the components with the
/// minimal symmetric KL-distance (applied only to the q/p-dimension) until the
/// required number of components is reached.
/// @param cmpCache the component collection
/// @param maxCmpsAfterMerge the number of components we want to reach
/// @param surface the surface type on which the components are
void reduceMixtureWithKLDistance(std::vector<GsfComponent> &cmpCache,
                                 std::size_t maxCmpsAfterMerge,
                                 const Surface &surface);

}  // namespace Acts
