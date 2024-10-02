// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/DBScan.hpp"

#include <map>
#include <unordered_map>
#include <vector>

namespace Acts {

/// Clusters seed based on their direction, their Z impact parameter and their
/// momentum using DBScan
///
/// @param input Input parameters for the clustering (phi, eta, z, Pt)
/// @param epsilon Maximum distance between 2 seed to be clustered
/// @param minPoints Minimum number of seeds to create a cluster
/// @return an unordered map representing the clusters, the keys the ID of the primary seed of each cluster and the stored value a vector of seed IDs.
std::vector<std::vector<std::size_t>> dbscanSeedClustering(
    const std::vector<std::array<double, 4>>& input, float epsilon = 0.03,
    int minPoints = 2) {
  // Initialize a DBScan of dimension 4 (phi, eta, z, Pt)
  using DBSCAN = Acts::DBScan<4, double, 4>;
  DBSCAN dbscan(epsilon, minPoints, true);

  // Cluster track with DBScan
  std::vector<int> clusterAssignments;
  std::size_t clusterNb = dbscan.cluster(input, clusterAssignments);

  // Prepare the output
  std::vector<std::vector<std::size_t>> cluster(clusterNb,
                                                std::vector<std::size_t>());
  for (std::size_t iD = 0; iD < input.size(); iD++) {
    int clusterID = clusterAssignments[iD];
    cluster[clusterID].push_back(iD);
  }

  return cluster;
}

}  // namespace Acts
