// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Hashing/HashingAnnoy.hpp"
#include "Acts/Seeding/detail/UtilityFunctions.hpp"

#include <memory>
#include <vector>

namespace Acts {
// constructor
template <typename external_spacepoint_t, typename SpacePointContainer>
HashingAlgorithm<external_spacepoint_t, SpacePointContainer>::HashingAlgorithm(
    const HashingAlgorithmConfig& cfg)
    : m_cfg(cfg) {
  if (m_cfg.bucketSize <= 0) {
    throw std::invalid_argument("Invalid bucket size");
  }
}

// function to create the buckets of spacepoints.
template <typename external_spacepoint_t, typename SpacePointContainer>
template <typename collection_t>
void HashingAlgorithm<external_spacepoint_t, SpacePointContainer>::execute(
    SpacePointContainer& spacePoints, AnnoyModel* annoyModel,
    collection_t& outputCollection) const {
  // Define a map to store the buckets of spacepoints a set is used to exclude
  // duplicates
  using map_t = std::map<unsigned int, std::set<external_spacepoint_t>>;

  // Get the number of space points
  const std::size_t nSpacePoints = spacePoints.size();

  // Get the bucket size, zBins, and phiBins from the configuration
  const unsigned int bucketSize = m_cfg.bucketSize;
  const unsigned int zBins = m_cfg.zBins;
  const unsigned int phiBins = m_cfg.phiBins;

  // Get the layer selection from the configuration
  const double layerRMin = m_cfg.layerRMin;
  const double layerRMax = m_cfg.layerRMax;
  const double layerZMin = m_cfg.layerZMin;
  const double layerZMax = m_cfg.layerZMax;

  // Create an instance of the HashingAnnoy class
  auto annoyHashingInstance = std::make_unique<
      HashingAnnoy<external_spacepoint_t, SpacePointContainer>>();

  // Compute the buckets of spacepoints using the Annoy model
  annoyHashingInstance->computeSpacePointsBuckets(
      annoyModel, spacePoints, bucketSize, zBins, phiBins, layerRMin, layerRMax,
      layerZMin, layerZMax);

  // Get the map of buckets and the number of buckets
  map_t bucketsSPMap = annoyHashingInstance->m_bucketsSPMap;
  auto nBuckets = static_cast<unsigned int>(bucketsSPMap.size());

  // Check if the number of buckets is greater than the number of space points
  if (nBuckets > nSpacePoints) {
    throw std::runtime_error("More buckets than the number of Space Points");
  }

  // Convert the map to a SpacePointContainer of SpacePointContainers
  for (unsigned int bucketIdx = 0; bucketIdx < nBuckets; bucketIdx++) {
    // Find the bucket in the map
    typename map_t::iterator iterator = bucketsSPMap.find(bucketIdx);
    if (iterator == bucketsSPMap.end()) {
      throw std::runtime_error("Not every bucket have been found");
    }

    // Get the set of spacepoints in the bucket
    std::set<external_spacepoint_t> bucketSet = iterator->second;

    // Convert the set to a SpacePointContainer
    SpacePointContainer bucket;
    for (external_spacepoint_t spacePoint : bucketSet) {
      bucket.push_back(spacePoint);
    }

    // Add the bucket container to the output collection
    Acts::detail::pushBackOrInsertAtEnd(outputCollection,
                                        SpacePointContainer{bucket});
  }
}

}  // namespace Acts
