// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"

#include <cstdint>

#include <annoy/annoylib.h>
#include <annoy/kissrandom.h>

namespace Acts {

// constructor
template <typename SpacePointContainer>
HashingTrainingAlgorithm<SpacePointContainer>::HashingTrainingAlgorithm(
    const Acts::HashingTrainingConfig& cfg)
    : m_cfg(cfg) {
  if (m_cfg.f <= 0) {
    throw std::invalid_argument("Invalid f, f must be positive");
  }
  if (m_cfg.annoySeed <= 0) {
    throw std::invalid_argument(
        "Invalid Annoy random seed, Annoy random seed must be positive");
  }
}

template <typename SpacePointContainer>
AnnoyModel HashingTrainingAlgorithm<SpacePointContainer>::execute(
    SpacePointContainer spacePoints) const {
  const unsigned int annoySeed = m_cfg.annoySeed;
  const std::int32_t f = m_cfg.f;

  auto annoyModel = AnnoyModel(f);

  using Scalar = Acts::ActsScalar;

  annoyModel.set_seed(annoySeed);

  unsigned int spacePointIndex = 0;
  // Add spacePoints parameters to Annoy
  for (const auto& spacePoint : spacePoints) {
    Scalar x = spacePoint->x() / Acts::UnitConstants::mm;
    Scalar y = spacePoint->y() / Acts::UnitConstants::mm;

    // Helix transform
    Scalar phi = atan2(y, x);

    std::vector<double> vec(f);
    // Avoid potential null pointer dereference
    if (f >= 1) {
      vec[0] = phi;
    }
    if (f >= 2) {
      Scalar z = spacePoint->z() / Acts::UnitConstants::mm;
      Scalar r2 = x * x + y * y;
      Scalar rho = sqrt(r2 + z * z);
      Scalar theta = acos(z / rho);
      Scalar eta = -log(tan(0.5 * theta));
      vec[1] = eta;
    }

    annoyModel.add_item(spacePointIndex, vec.data());
    spacePointIndex++;
  }

  unsigned int nTrees = 2 * f;

  annoyModel.build(nTrees);

  return annoyModel;
}

}  // namespace Acts
