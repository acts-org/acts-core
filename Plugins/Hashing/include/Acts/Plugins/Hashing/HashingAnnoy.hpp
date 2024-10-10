// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
