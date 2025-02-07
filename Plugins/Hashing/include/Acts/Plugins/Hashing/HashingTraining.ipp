// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/AngleHelpers.hpp"

#include <cmath>
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

  annoyModel.set_seed(annoySeed);

  unsigned int spacePointIndex = 0;
  // Add spacePoints parameters to Annoy
  for (const auto& spacePoint : spacePoints) {
    double x = spacePoint->x() / Acts::UnitConstants::mm;
    double y = spacePoint->y() / Acts::UnitConstants::mm;

    // Helix transform
    double phi = std::atan2(y, x);

    std::vector<double> vec(f);
    // Avoid potential null pointer dereference
    if (f >= 1) {
      vec[0] = phi;
    }
    if (f >= 2) {
      double z = spacePoint->z() / Acts::UnitConstants::mm;
      double r2 = x * x + y * y;
      double rho = std::sqrt(r2 + z * z);
      double theta = std::acos(z / rho);
      double eta = Acts::AngleHelpers::etaFromTheta(theta);
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
