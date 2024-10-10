// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace ActsExamples {

enum class TrackMatchClassification {
  Unknown = 0,
  /// The track is associated to a truth particle
  Matched,
  /// The track is associated to a truth particle, but the track is not unique
  Duplicate,
  /// The track cannot be uniquely associated to a truth particle
  Fake,
};

struct TrackMatchEntry {
  TrackMatchClassification classification{TrackMatchClassification::Unknown};

  std::optional<SimBarcode> particle;

  /// Number of hits on the track that are associated to a particle
  /// Sorted by decreasing number of hits
  std::vector<ParticleHitCount> contributingParticles;
};

struct ParticleMatchEntry {
  std::optional<TrackIndexType> track;
  std::uint32_t duplicates{};
  std::uint32_t fakes{};
};

using TrackParticleMatching = std::map<TrackIndexType, TrackMatchEntry>;
using ParticleTrackMatching = std::map<SimBarcode, ParticleMatchEntry>;

}  // namespace ActsExamples
