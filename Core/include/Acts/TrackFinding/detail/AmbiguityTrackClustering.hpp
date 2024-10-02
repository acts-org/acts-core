// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Acts::detail {

/// Clusterise tracks based on shared hits
///
/// @param trackMap : Multimap storing pair of track ID and vector of measurement ID. The keys are the number of measurement and are just there to facilitate the ordering.
/// @return an unordered map representing the clusters, the keys the ID of the primary track of each cluster and the store a vector of track IDs.
std::unordered_map<std::size_t, std::vector<std::size_t>>
clusterDuplicateTracks(
    const std::multimap<int, std::pair<std::size_t, std::vector<std::size_t>>>&
        trackMap);

}  // namespace Acts::detail
