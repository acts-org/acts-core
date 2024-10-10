// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Tests/CommonHelpers/BenchmarkTools.hpp"

#include <iostream>
#include <type_traits>

using namespace Acts;

class BenchmarkSourceLink final {
 public:
  using Index = std::uint32_t;

  /// Construct from geometry identifier and index.
  constexpr BenchmarkSourceLink(Acts::GeometryIdentifier gid, Index idx)
      : m_geometryId(gid), m_index(idx) {}

  BenchmarkSourceLink() = default;
  BenchmarkSourceLink(const BenchmarkSourceLink&) = default;
  BenchmarkSourceLink(BenchmarkSourceLink&&) = default;
  BenchmarkSourceLink& operator=(const BenchmarkSourceLink&) = default;
  BenchmarkSourceLink& operator=(BenchmarkSourceLink&&) = default;

  /// Access the index.
  constexpr Index index() const { return m_index; }

  Acts::GeometryIdentifier geometryId() const { return m_geometryId; }

 private:
  Acts::GeometryIdentifier m_geometryId;
  Index m_index = 0;

  friend bool operator==(const BenchmarkSourceLink& lhs,
                         const BenchmarkSourceLink& rhs) {
    return (lhs.geometryId() == rhs.geometryId()) &&
           (lhs.m_index == rhs.m_index);
  }
};

int main(int /*argc*/, char** /*argv[]*/) {
  std::size_t n = 100000;

  VectorMultiTrajectory mtj;

  GeometryIdentifier gid;
  gid.setVolume(5);
  gid.setLayer(3);
  gid.setSensitive(1);

  static_assert(sizeof(BenchmarkSourceLink) <= ACTS_SOURCELINK_SBO_SIZE);

  static_assert(std::is_trivially_move_constructible_v<BenchmarkSourceLink>);

  BenchmarkSourceLink bsl{gid, 1234};

  std::cout << "Creating source link" << std::endl;
  auto sourceLinkConstruction = Acts::Test::microBenchmark(
      [&]() {
        SourceLink sl{bsl};
        return sl;
      },
      n);
  std::cout << "  " << sourceLinkConstruction << std::endl;

  std::vector<SourceLink> inputs;
  inputs.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    inputs.emplace_back(bsl);
  }

  std::cout << "Copy construct source link" << std::endl;
  auto copyConstructSourceLink = Acts::Test::microBenchmark(
      [&](const SourceLink& input) {
        SourceLink copy{input};
        return copy;
      },
      inputs);
  std::cout << copyConstructSourceLink << std::endl;

  std::cout << "Copy then move construct source link" << std::endl;
  auto copyMoveConstructSourceLink = Acts::Test::microBenchmark(
      [&](const SourceLink& input) {
        SourceLink copy{input};
        SourceLink mv{std::move(copy)};
        return mv;
      },
      inputs);
  std::cout << copyMoveConstructSourceLink << std::endl;

  std::cout << "Optional assignment" << std::endl;
  auto opt_assignment = Acts::Test::microBenchmark(
      [&]() {
        SourceLink sl{bsl};
        // ts.setUncalibratedSourceLink(std::move(sl));
        std::optional<SourceLink> opt;
        opt = std::move(sl);
        return opt;
      },
      n);
  std::cout << opt_assignment << std::endl;

  // Measure track state creation
  std::cout << "Create track state" << std::endl;
  auto create_track_state = Acts::Test::microBenchmark(
      [&]() {
        auto ts = mtj.makeTrackState(TrackStatePropMask::None);
        return ts;
      },
      n / 10);
  std::cout << create_track_state << std::endl;

  std::cout << "Assign source link to track state" << std::endl;
  auto assignSourceLink = Acts::Test::microBenchmark(
      [&]() {
        SourceLink sl{bsl};
        auto ts = mtj.makeTrackState(TrackStatePropMask::None);
        ts.setUncalibratedSourceLink(std::move(sl));
        return ts;
      },
      n / 10);
  std::cout << assignSourceLink << std::endl;

  return 0;
}
