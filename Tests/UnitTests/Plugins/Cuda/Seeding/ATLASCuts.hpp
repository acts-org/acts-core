// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Seeding/IExperimentCuts.hpp"

#include <algorithm>

namespace Acts {
template <typename SpacePoint>
class ATLASCuts : public IExperimentCuts<SpacePoint> {
 public:
  /// Returns seed weight bonus/malus depending on detector considerations.
  /// @param bottom bottom space point of the current seed
  /// @param middle middle space point of the current seed
  /// @param top top space point of the current seed
  /// @return seed weight to be added to the seed's weight
  float seedWeight(const InternalSpacePoint<SpacePoint>& bottom,
                   const InternalSpacePoint<SpacePoint>& middle,
                   const InternalSpacePoint<SpacePoint>& top) const;
  /// @param weight the current seed weight
  /// @param bottom bottom space point of the current seed
  /// @param middle middle space point of the current seed
  /// @param top top space point of the current seed
  /// @return true if the seed should be kept, false if the seed should be
  /// discarded
  bool singleSeedCut(float weight, const InternalSpacePoint<SpacePoint>& bottom,
                     const InternalSpacePoint<SpacePoint>&,
                     const InternalSpacePoint<SpacePoint>&) const;

  /// @param seedCandidates contains collection of seed candidates created for one middle
  /// space point in a std::tuple format
  /// @return vector of seed candidates that pass the cut
  std::vector<typename CandidatesForMiddleSp<
      const InternalSpacePoint<SpacePoint>>::value_type>
  cutPerMiddleSP(std::vector<typename CandidatesForMiddleSp<
                     const InternalSpacePoint<SpacePoint>>::value_type>
                     seedCandidates) const override;
};

template <typename SpacePoint>
float ATLASCuts<SpacePoint>::seedWeight(
    const InternalSpacePoint<SpacePoint>& bottom,
    const InternalSpacePoint<SpacePoint>&,
    const InternalSpacePoint<SpacePoint>& top) const {
  float weight = 0;
  if (bottom.radius() > 150) {
    weight = 400;
  }
  if (top.radius() < 150) {
    weight = 200;
  }
  return weight;
}

template <typename SpacePoint>
bool ATLASCuts<SpacePoint>::singleSeedCut(
    float weight, const InternalSpacePoint<SpacePoint>& b,
    const InternalSpacePoint<SpacePoint>&,
    const InternalSpacePoint<SpacePoint>&) const {
  return !(b.radius() > 150. && weight < 380.);
}

template <typename SpacePoint>
std::vector<typename CandidatesForMiddleSp<
    const InternalSpacePoint<SpacePoint>>::value_type>
ATLASCuts<SpacePoint>::cutPerMiddleSP(
    std::vector<typename CandidatesForMiddleSp<
        const InternalSpacePoint<SpacePoint>>::value_type>
        seedCandidates) const {
  std::vector<typename CandidatesForMiddleSp<
      const InternalSpacePoint<SpacePoint>>::value_type>
      newSeedsVector;
  if (seedCandidates.size() <= 1) {
    return seedCandidates;
  }

  newSeedsVector.push_back(std::move(seedCandidates[0]));
  std::size_t itLength = std::min(seedCandidates.size(), std::size_t{5});
  // don't cut first element
  for (std::size_t i(1); i < itLength; i++) {
    float weight = seedCandidates[i].weight;
    const auto& bottom = seedCandidates[i].bottom;
    if (weight > 200. || bottom->radius() > 43.) {
      newSeedsVector.push_back(std::move(seedCandidates[i]));
    }
  }
  return newSeedsVector;
}
}  // namespace Acts
