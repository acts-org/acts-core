// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

// Local include(s).
#include "TestHostCuts.hpp"

float TestHostCuts::seedWeight(
    const Acts::InternalSpacePoint<TestSpacePoint>& bottom,
    const Acts::InternalSpacePoint<TestSpacePoint>&,
    const Acts::InternalSpacePoint<TestSpacePoint>& top) const {
  float weight = 0;
  if (bottom.radius() > 150) {
    weight = 400;
  }
  if (top.radius() < 150) {
    weight = 200;
  }
  return weight;
}

bool TestHostCuts::singleSeedCut(
    float weight, const Acts::InternalSpacePoint<TestSpacePoint>& b,
    const Acts::InternalSpacePoint<TestSpacePoint>&,
    const Acts::InternalSpacePoint<TestSpacePoint>&) const {
  return !(b.radius() > 150. && weight < 380.);
}

std::vector<typename Acts::CandidatesForMiddleSp<
    const Acts::InternalSpacePoint<TestSpacePoint>>::value_type>
TestHostCuts::cutPerMiddleSP(
    std::vector<typename Acts::CandidatesForMiddleSp<
        const Acts::InternalSpacePoint<TestSpacePoint>>::value_type>
        seedCandidates) const {
  std::vector<typename Acts::CandidatesForMiddleSp<
      const Acts::InternalSpacePoint<TestSpacePoint>>::value_type>
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
    if (weight > 200. or bottom->radius() > 43.) {
      newSeedsVector.push_back(std::move(seedCandidates[i]));
    }
  }
  return newSeedsVector;
}
