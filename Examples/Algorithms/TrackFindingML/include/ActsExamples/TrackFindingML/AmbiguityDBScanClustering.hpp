// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackContainerFrontendConcept.hpp"
#include "Acts/TrackFinding/detail/AmbiguityTrackClustering.hpp"
#include "Acts/Utilities/DBScan.hpp"

#include <map>
#include <unordered_map>
#include <vector>

namespace Acts {

/// Clusterise tracks based on shared hits
///
/// @param trackMap Multimap storing pair of track ID and vector of measurement ID. The keys are the number of measurement and are just there to facilitate the ordering.
/// @param tracks Track container with all the track to be clustered
/// @param epsilon Maximum distance between 2 tracks to be clustered
/// @param minPoints Minimum number of tracks to create a cluster
/// @return an unordered map representing the clusters, the keys the ID of the primary track of each cluster and the store a vector of track IDs.
template <TrackContainerFrontend track_container_t>
std::unordered_map<std::size_t, std::vector<std::size_t>> dbscanTrackClustering(
    std::multimap<int, std::pair<std::size_t, std::vector<std::size_t>>>&
        trackMap,
    const track_container_t& tracks, float epsilon = 0.07, int minPoints = 2) {
  // Unordered map associating a vector with all the track ID of a cluster to
  // the ID of the first track of the cluster
  std::unordered_map<std::size_t, std::vector<std::size_t>> cluster;
  // Unordered map associating hits to the ID of the first track of the
  // different clusters.
  std::unordered_map<std::size_t, std::size_t> hitToTrack;

  // Initialize a DBScan of dimension 4 (phi, eta, z, Pt)
  using DBSCAN = Acts::DBScan<4, double, 4>;
  DBSCAN dbscan(epsilon, minPoints, true);

  std::vector<std::array<double, 4>> data;
  std::size_t trackID = 0;
  std::vector<int> clusterAssignments;

  // Get the input feature of the network for all the tracks
  for (const auto& [key, val] : trackMap) {
    auto traj = tracks.getTrack(val.first);
    data.push_back({Acts::VectorHelpers::eta(traj.momentum()),
                    Acts::VectorHelpers::phi(traj.momentum())});
  }
  std::size_t clusterNb = dbscan.cluster(data, clusterAssignments);

  // Cluster track with DBScan
  std::vector<
      std::multimap<int, std::pair<std::size_t, std::vector<std::size_t>>>>
      dbscanClusters(clusterNb);
  for (const auto& [key, val] : trackMap) {
    std::size_t clusterID = clusterAssignments[trackID];
    dbscanClusters[clusterID].emplace(key, val);
    trackID++;
  }

  // Perform a subClustering of the DBScan cluster using the measurement ID
  // clustering
  for (const auto& dbscanCluster : dbscanClusters) {
    auto subCluster = Acts::detail::clusterDuplicateTracks(dbscanCluster);
    cluster.merge(subCluster);
    if (!subCluster.empty()) {
      std::cout << "Overlapping track ID, there must be an error" << std::endl;
    }
  }
  return cluster;
}

}  // namespace Acts
