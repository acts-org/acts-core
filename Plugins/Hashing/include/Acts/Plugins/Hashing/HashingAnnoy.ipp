// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"

#include <map>
#include <set>
#include <vector>

#include <annoy/annoylib.h>
#include <annoy/kissrandom.h>

namespace Acts {

template <typename external_spacepoint_t, typename SpacePointContainer>
void HashingAnnoy<external_spacepoint_t, SpacePointContainer>::
    computeSpacePointsBuckets(const AnnoyModel* annoyModel,
                              const SpacePointContainer& spacePoints,
                              const unsigned int bucketSize,
                              const unsigned int zBins,
                              const unsigned int phiBins,
                              const double layerRMin, const double layerRMax,
                              const double layerZMin, const double layerZMax) {
  using Scalar = Acts::ActsScalar;

  static thread_local std::vector<std::set<external_spacepoint_t>>
      bucketsSetSPMap;
  bucketsSetSPMap.clear();

  unsigned int nBins = 0;
  if (zBins > 0) {
    nBins = zBins;
  } else if (phiBins > 0) {
    nBins = phiBins;
  } else {
    throw std::runtime_error("No bins defined");
  }
  bucketsSetSPMap.reserve(nBins);

  // Create the set of spacePoints for each bucket
  for (unsigned int binIndex = 0; binIndex < nBins; binIndex++) {
    static thread_local std::set<external_spacepoint_t> bucket;
    bucket.clear();
    bucketsSetSPMap.push_back(bucket);
  }

  // Function to check if a spacePoint is inside the layer
  auto layerSelection = [&layerRMin, &layerRMax, &layerZMin, &layerZMax](
                            double r2, double z) {
    bool isInside =
        (r2 >= layerRMin * layerRMin && r2 <= layerRMax * layerRMax) &&
        (z >= layerZMin && z <= layerZMax);
    return isInside;
  };

  // Functions to get the bin index
  auto getBinIndexZ = [&zBins, &layerZMin, &layerZMax](Scalar z) {
    Scalar binSize = (layerZMax - layerZMin) / zBins;
    auto binIndex = static_cast<int>((z - layerZMin + 0.5 * binSize) / binSize);
    return binIndex;
  };

  auto getBinIndexPhi = [&phiBins](Scalar phi) {
    Scalar binSize = 2 * M_PI / phiBins;
    auto binIndex = static_cast<int>((phi + M_PI) / binSize);
    return binIndex;
  };

  // Function pointers to a unified bin index function for z and phi
  auto getBinIndex = [&zBins, &phiBins, &getBinIndexZ, &getBinIndexPhi](
                         Scalar z, Scalar phi) -> int {
    if (zBins > 0) {
      return getBinIndexZ(z);
    } else if (phiBins > 0) {
      return getBinIndexPhi(phi);
    } else {
      throw std::runtime_error("No bins defined");
    }
  };

  // Loop over spacePoints
  for (unsigned int spacePointIndex = 0; spacePointIndex < spacePoints.size();
       spacePointIndex++) {
    external_spacepoint_t spacePoint = spacePoints[spacePointIndex];
    Scalar x = spacePoint->x() / Acts::UnitConstants::mm;
    Scalar y = spacePoint->y() / Acts::UnitConstants::mm;
    Scalar z = spacePoint->z() / Acts::UnitConstants::mm;

    // Helix transform
    if (Scalar r2 = x * x + y * y; !layerSelection(r2, z)) {
      continue;
    }

    Scalar phi = atan2(y, x);

    int binIndex = getBinIndex(z, phi);
    if (binIndex < 0 || static_cast<unsigned int>(binIndex) >= nBins) {
      throw std::runtime_error("binIndex outside of bins covering");
    }

    std::vector<unsigned int> bucketIds;

    /// Get the bucketSize closest spacePoints
    annoyModel->get_nns_by_item(spacePointIndex, bucketSize, -1, &bucketIds,
                                nullptr);

    for (const unsigned int& bucketSpacePointIndex : bucketIds) {
      bucketsSetSPMap[static_cast<unsigned int>(binIndex)].insert(
          spacePoints.at(bucketSpacePointIndex));
    }
  }

  unsigned int n_buckets = 0;
  for (unsigned int binIndex = 0; binIndex < nBins; binIndex++) {
    if (bucketsSetSPMap[binIndex].size() > 0) {
      m_bucketsSPMap[n_buckets] = bucketsSetSPMap[binIndex];
      n_buckets++;
    }
  }
}

}  // namespace Acts
