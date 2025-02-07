// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
