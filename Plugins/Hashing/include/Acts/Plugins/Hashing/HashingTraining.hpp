// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Hashing/AnnoyForwardDeclarations.hpp"
#include "Acts/Plugins/Hashing/HashingTrainingConfig.hpp"

namespace Acts {

template <typename SpacePointContainer>
class HashingTrainingAlgorithm {
 public:
  explicit HashingTrainingAlgorithm(const HashingTrainingConfig &cfg);

  HashingTrainingAlgorithm() = default;
  HashingTrainingAlgorithm(
      const HashingTrainingAlgorithm<SpacePointContainer> &) = delete;
  HashingTrainingAlgorithm<SpacePointContainer> &operator=(
      const HashingTrainingAlgorithm<SpacePointContainer> &) = default;

  AnnoyModel execute(SpacePointContainer spacePoints) const;

  // / Get readonly access to the config parameters
  const Acts::HashingTrainingConfig &config() const { return m_cfg; }

 private:
  HashingTrainingConfig m_cfg;
};

}  // namespace Acts

#include "Acts/Plugins/Hashing/HashingTraining.ipp"
