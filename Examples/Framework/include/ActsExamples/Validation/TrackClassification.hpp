// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsFatras/EventData/Barcode.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace ActsExamples {
struct Trajectories;

/// Associate a particle to its hit count within a proto track.
struct ParticleHitCount {
  ActsFatras::Barcode particleId;
  std::size_t hitCount;
};

/// Identify all particles that contribute to the proto track.
///
/// @param[in] hitParticlesMap Map hit indices to contributing particles
/// @param[in] protoTrack The proto track to classify
/// @param[out] particleHitCounts List of contributing particles
///
/// The list of contributing particles is ordered according to their hit count,
/// i.e. the first element is the majority particle that contributes the most
/// hits to the track. There can be both hits without a generating particle
/// (noise hits) and hits that have more than one generating particle. The sum
/// of the particle hit count must not be identical to the size of the proto
/// track.
void identifyContributingParticles(
    const IndexMultimap<ActsFatras::Barcode>& hitParticlesMap,
    const ProtoTrack& protoTrack,
    std::vector<ParticleHitCount>& particleHitCounts);

/// Identify all particles that contribute to a trajectory.
///
/// @param[in] hitParticlesMap Map hit indices to contributing particles
/// @param[in] trajectories The input trajectories to classify
/// @param[in] trajectoryTip Which trajectory in the trajectories to use
/// @param[out] particleHitCounts List of contributing particles
///
/// See `identifyContributingParticles` for proto tracks for further
/// information.
void identifyContributingParticles(
    const IndexMultimap<ActsFatras::Barcode>& hitParticlesMap,
    const Trajectories& trajectories, std::size_t trajectoryTip,
    std::vector<ParticleHitCount>& particleHitCounts);

void identifyContributingParticles(
    const IndexMultimap<ActsFatras::Barcode>& hitParticlesMap,
    const ConstTrackContainer::ConstTrackProxy& track,
    std::vector<ParticleHitCount>& particleHitCounts);

}  // namespace ActsExamples
