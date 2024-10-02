// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/KDTree.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace Acts {
/// @brief A general implementation of an N dimensional DBScan clustering algorithm.
///
/// This is a general implementation of an N dimensional DBScan clustering
/// algorithm. The DBScan algorithm uses density information to cluster together
/// points that are close to each other.
///
/// For each point, we will look for the neighbours that are within the epsilon
/// radius. If the number of neighbours is greater than the minimum number of
/// points, we will start a new cluster and assign the current point to it. We
/// will then look for the neighbours of the neighbours and assign them to the
/// current cluster if they are not already assigned to a cluster. If the
/// neighbours have itself more than the minimum number of points as neighbours,
/// we will repeat the process on those neighbours.
///
/// To speed up the search for the neighbours, we use the KDTree implemented in
/// ACTS. It performs a range search in the orthogonal hypercube with a length
/// of 2 epsilon. An extra cut is used to only keep the neighbours that are
/// within the epsilon radius.
///
/// @tparam kDims The number of dimensions.
/// @tparam scalar_t The scalar type used to construct position vectors.
/// @tparam kLeafSize The maximum number of points in a leaf node of the KDTree.
template <std::size_t kDims, typename scalar_t = double,
          std::size_t kLeafSize = 4>
class DBScan {
 public:
  // The type of coordinates for points.
  using Point = std::array<scalar_t, kDims>;

  // The type of a vector of coordinate.
  using VectorPoints = std::vector<Point>;

  // The type to pair the points with an ID.
  using Pair = std::pair<Point, std::size_t>;

  // The type of a vector of coordinate-ID pairs.
  using VectorPairs = std::vector<Pair>;

  // KDTree used before the DBScan algorithm to find the neighbours.
  using Tree = KDTree<kDims, std::size_t, scalar_t, std::array, kLeafSize>;

  // Remove the default constructor.
  DBScan() = delete;

  /// @brief Construct the DBScan algorithm with a given epsilon and minPoints.
  ///
  /// @param epsilon The epsilon radius used to find the neighbours.
  /// @param minPoints The minimum number of points to form a cluster.
  /// @param onePointCluster If true, all the noise points are considered as
  /// individual one point clusters.
  DBScan(scalar_t epsilon = 1.0, std::size_t minPoints = 1,
         bool onePointCluster = false)
      : m_eps(epsilon),
        m_minPoints(minPoints),
        m_onePointCluster(onePointCluster) {}

  /// @brief Cluster the input points.
  ///
  /// This function implements the main loop of the DBScan algorithm.
  /// It loops over all the point and will try to start new cluster
  /// if it finds points that have yet to be clustered.
  ///
  /// @param inputPoints The input points to cluster.
  /// @param clusteredPoints Vector containing the cluster ID of each point.
  /// @return The number of clusters (excluding noise if onePointCluster==False).
  ///
  int cluster(const VectorPoints& inputPoints,
              std::vector<int>& clusteredPoints) {
    // Transform the initial vector of input point to a vector of pairs
    // with the index of the point in the initial vector.
    VectorPairs inputPointsWithIndex;
    for (std::size_t id = 0; id < inputPoints.size(); id++) {
      inputPointsWithIndex.push_back(std::make_pair(inputPoints[id], id));
    }
    // Build the KDTree with the input points.
    Tree tree = Tree(std::move(inputPointsWithIndex));

    // Initialize the cluster ID to 0.
    int clusterID = 0;
    // By default all the points are considered as noise.
    clusteredPoints = std::vector<int>(inputPoints.size(), -1);

    // Loop over all the points
    for (std::size_t id = 0; id < inputPoints.size(); id++) {
      // If the point is already assigned to a cluster, skip it.
      if (clusteredPoints[id] != -1) {
        continue;
      }
      // If not we try to build a new cluster
      std::vector<std::size_t> pointToProcess{id};
      expandCluster(tree, inputPoints, pointToProcess, clusteredPoints,
                    clusterID);
      // If the cluster has been created, increment the cluster ID.
      if (clusteredPoints[id] != -1) {
        clusterID++;
      }
    }
    if (m_onePointCluster) {
      // If noise is present and onePointCluster is true, all the noise points
      // are considered as individual one point clusters. Loop over all the
      // points in the KDTree.
      for (auto& cluster : clusteredPoints) {
        // If the point is assigned to noise, assign it to a new cluster.
        if (cluster == -1) {
          cluster = clusterID;
          clusterID++;
        }
      }
    }
    return clusterID;
  }

 private:
  /// @brief Extend the cluster.
  ///
  /// This function will extend the cluster by finding all the neighbours of the
  /// current point and assign them to the current cluster.
  /// The KDTree is used to find the neighbours and an extra cut is used to only
  /// keep the neighbours that are within the epsilon radius.
  ///
  /// @param tree The KDTree containing all the points.
  /// @param inputPoints The vector containing the input points.
  /// @param pointsToProcess The vector containing the ids of the points that need to be
  /// processed.
  /// @param clusteredPoints Vector containing the cluster ID of each point.
  /// @param clusterID The ID of the current cluster.
  ///
  void expandCluster(const Tree& tree, const VectorPoints& inputPoints,
                     const std::vector<std::size_t>& pointsToProcess,
                     std::vector<int>& clusteredPoints, const int clusterID) {
    // Loop over all the points that need to be process.
    for (const auto id : pointsToProcess) {
      // Lets look for the neighbours of the current point.
      const Point currentPoint = inputPoints[id];
      std::vector<std::size_t> neighbours;
      // We create the range in which we will look for the neighbours (an
      // hypercube with a length of 2 epsilon).
      typename Tree::range_t range;
      for (std::size_t dim = 0; dim < kDims; dim++) {
        range[dim] = Range1D<scalar_t>(currentPoint[dim] - m_eps,
                                       currentPoint[dim] + m_eps);
      }
      // We use the KDTree to find the neighbours.
      // An extra cut needs to be applied to only keep the neighbours that
      // are within the epsilon radius.
      tree.rangeSearchMapDiscard(
          range, [this, &neighbours, currentPoint](
                     const typename Tree::coordinate_t& pos,
                     const typename Tree::value_t& val) {
            scalar_t distance = 0;
            for (std::size_t dim = 0; dim < kDims; dim++) {
              distance += (pos[dim] - currentPoint[dim]) *
                          (pos[dim] - currentPoint[dim]);
            }
            if (distance <= m_eps * m_eps) {
              neighbours.push_back(val);
            }
          });
      std::size_t nNeighbours = neighbours.size();
      // If a cluster has already been started we add the neighbours to it
      if (clusteredPoints[id] != -1) {
        updateNeighbours(neighbours, clusteredPoints, clusterID);
      }
      if (nNeighbours >= m_minPoints) {
        // If the cluster has not been started yet and we have enough
        // neighbours, we start the cluster and assign the current point and its
        // neighbours to it.
        if (clusteredPoints[id] == -1) {
          clusteredPoints[id] = clusterID;
          updateNeighbours(neighbours, clusteredPoints, clusterID);
        }
        // Try to extend the cluster with the neighbours.
        expandCluster(tree, inputPoints, neighbours, clusteredPoints,
                      clusterID);
      }
    }
  }

  /// @brief Update the neighbours.
  ///
  /// This function will remove the neighbours that are already assigned to a
  /// cluster and assign the remaining ones to the current cluster.
  ///
  /// @param neighbours The vector containing the ids of the neighbours.
  /// @param clusteredPoints Vector containing the cluster ID of each point.
  /// @param clusterID The ID of the current cluster.
  ///
  void updateNeighbours(std::vector<std::size_t>& neighbours,
                        std::vector<int>& clusteredPoints,
                        const int clusterID) {
    neighbours.erase(std::remove_if(neighbours.begin(), neighbours.end(),
                                    [&clusteredPoints](int i) {
                                      return clusteredPoints[i] != -1;
                                    }),
                     neighbours.end());

    for (const auto& neighbour : neighbours) {
      clusteredPoints[neighbour] = clusterID;
    }
  }

  // The epsilon radius used to find the neighbours.
  scalar_t m_eps;
  // The minimum number of points to form a cluster.
  std::size_t m_minPoints = 1;
  // If true, all the noise points are considered as individual one point
  // clusters.
  bool m_onePointCluster = false;
};

}  // namespace Acts
