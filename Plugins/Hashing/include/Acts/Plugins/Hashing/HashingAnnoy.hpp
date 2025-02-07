// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/Hashing/AnnoyForwardDeclarations.hpp"

#include <map>
#include <set>

namespace Acts {

template <typename external_spacepoint_t, typename SpacePointContainer>
class HashingAnnoy {
 public:
  void computeSpacePointsBuckets(
      const AnnoyModel* annoyModel, const SpacePointContainer& spacePoints,
      const unsigned int bucketSize, const unsigned int zBins,
      const unsigned int phiBins, const double layerRMin,
      const double layerRMax, const double layerZMin, const double layerZMax);
  std::map<unsigned int, std::set<external_spacepoint_t>> m_bucketsSPMap;
};
}  // namespace Acts

#include "Acts/Plugins/Hashing/HashingAnnoy.ipp"
