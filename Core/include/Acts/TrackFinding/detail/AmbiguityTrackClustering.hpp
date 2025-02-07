// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Acts::detail {

/// Cluster tracks based on shared hits.
///
/// In this algorithm we will loop through all the tracks by decreasing number
/// of measurements. Cluster are created when a new track is encountered that
/// doesn't share hits with the leading track of a previous cluster (with the
/// leading track defined as the track that lead to the cluster creation). If a
/// track shares hits with the leading track of a cluster, it is added to that
/// cluster. If a track shares hits with multiple clusters, it is associated to
/// the cluster with the leading track with the most hits.
///
/// @param trackMap : Multimap storing pair of track ID and vector of measurement ID. The keys are the number of measurement and are just there to facilitate the ordering.
/// @return an unordered map representing the clusters, the keys the ID of the primary track of each cluster and the store a vector of track IDs.
std::unordered_map<std::size_t, std::vector<std::size_t>>
clusterDuplicateTracks(
    const std::multimap<int, std::pair<std::size_t, std::vector<std::size_t>>>&
        trackMap);

}  // namespace Acts::detail
