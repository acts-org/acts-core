// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Validation/TrackClassification.hpp"

#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/Utilities/MultiIndex.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsExamples/Utilities/Range.hpp"

#include <algorithm>
#include <utility>

namespace {

/// Increase the hit count for the given particle id by one.
inline void increaseHitCount(
    std::vector<ActsExamples::ParticleHitCount>& particleHitCounts,
    ActsFatras::Barcode particleId) {
  // linear search since there is no ordering
  auto it = std::ranges::find_if(particleHitCounts, [=](const auto& phc) {
    return (phc.particleId == particleId);
  });
  // either increase count if we saw the particle before or add it
  if (it != particleHitCounts.end()) {
    it->hitCount += 1u;
  } else {
    particleHitCounts.push_back({particleId, 1u});
  }
}

/// Sort hit counts by decreasing values, i.e. majority particle comes first.
inline void sortHitCount(
    std::vector<ActsExamples::ParticleHitCount>& particleHitCounts) {
  std::ranges::sort(particleHitCounts, std::greater{},
                    [](const auto& p) { return p.hitCount; });
}

}  // namespace

void ActsExamples::identifyContributingParticles(
    const IndexMultimap<ActsFatras::Barcode>& hitParticlesMap,
    const ProtoTrack& protoTrack,
    std::vector<ActsExamples::ParticleHitCount>& particleHitCounts) {
  particleHitCounts.clear();

  for (auto hitIndex : protoTrack) {
    // register all particles that generated this hit
    for (auto hitParticle : makeRange(hitParticlesMap.equal_range(hitIndex))) {
      increaseHitCount(particleHitCounts, hitParticle.second);
    }
  }
  sortHitCount(particleHitCounts);
}

void ActsExamples::identifyContributingParticles(
    const IndexMultimap<ActsFatras::Barcode>& hitParticlesMap,
    const Trajectories& trajectories, std::size_t tip,
    std::vector<ParticleHitCount>& particleHitCounts) {
  particleHitCounts.clear();

  if (!trajectories.hasTrajectory(tip)) {
    return;
  }

  trajectories.multiTrajectory().visitBackwards(tip, [&](const auto& state) {
    // no truth info with non-measurement state
    if (!state.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
      return true;
    }
    // register all particles that generated this hit
    IndexSourceLink sl =
        state.getUncalibratedSourceLink().template get<IndexSourceLink>();
    auto hitIndex = sl.index();
    for (auto hitParticle : makeRange(hitParticlesMap.equal_range(hitIndex))) {
      increaseHitCount(particleHitCounts, hitParticle.second);
    }
    return true;
  });
  sortHitCount(particleHitCounts);
}

void ActsExamples::identifyContributingParticles(
    const IndexMultimap<ActsFatras::Barcode>& hitParticlesMap,
    const ConstTrackContainer::ConstTrackProxy& track,
    std::vector<ParticleHitCount>& particleHitCounts) {
  particleHitCounts.clear();

  for (const auto& state : track.trackStatesReversed()) {
    // no truth info with non-measurement state
    if (!state.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
      continue;
    }
    // register all particles that generated this hit
    IndexSourceLink sl =
        state.getUncalibratedSourceLink().template get<IndexSourceLink>();
    auto hitIndex = sl.index();
    for (auto hitParticle : makeRange(hitParticlesMap.equal_range(hitIndex))) {
      increaseHitCount(particleHitCounts, hitParticle.second);
    }
  }
  sortHitCount(particleHitCounts);
}
