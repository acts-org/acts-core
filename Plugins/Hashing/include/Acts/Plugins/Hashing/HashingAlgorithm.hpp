// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
