// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Hashing/AnnoyForwardDeclarations.hpp"
#include "Acts/Plugins/Hashing/HashingAlgorithmConfig.hpp"

namespace Acts {

template <typename external_spacepoint_t, typename SpacePointContainer>
class HashingAlgorithm {
 public:
  explicit HashingAlgorithm(const HashingAlgorithmConfig& cfg);

  HashingAlgorithm() = default;

  template <typename collection_t>
  void execute(SpacePointContainer& spacePoints, AnnoyModel* annoyModel,
               collection_t& outputCollection) const;

  /// Get readonly access to the config parameters
  const Acts::HashingAlgorithmConfig& config() const { return m_cfg; }

 private:
  HashingAlgorithmConfig m_cfg;
};

}  // namespace Acts

#include "Acts/Plugins/Hashing/HashingAlgorithm.ipp"
