// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Utilities/EventDataTransforms.hpp"

#include "Acts/EventData/SourceLink.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"

#include <algorithm>
#include <vector>

ActsExamples::ProtoTrack ActsExamples::seedToPrototrack(
    const ActsExamples::SimSeed& seed) {
  ProtoTrack track;
  track.reserve(seed.sp().size());
  for (const auto& spacePoints : seed.sp()) {
    for (const auto& slink : spacePoints->sourceLinks()) {
      const auto& islink = slink.get<IndexSourceLink>();
      track.emplace_back(islink.index());
    }
  }
  return track;
}

const ActsExamples::SimSpacePoint* ActsExamples::findSpacePointForIndex(
    ActsExamples::Index index, const SimSpacePointContainer& spacepoints) {
  auto match = [&](const SimSpacePoint& sp) {
    const auto& sls = sp.sourceLinks();
    return std::ranges::any_of(sls, [&](const auto& sl) {
      return sl.template get<IndexSourceLink>().index() == index;
    });
  };

  auto found = std::ranges::find_if(spacepoints, match);

  if (found == spacepoints.end()) {
    return nullptr;
  }

  return &(*found);
}

ActsExamples::SimSeed ActsExamples::prototrackToSeed(
    const ActsExamples::ProtoTrack& track,
    const ActsExamples::SimSpacePointContainer& spacepoints) {
  auto findSpacePoint = [&](ActsExamples::Index index) {
    auto found = findSpacePointForIndex(index, spacepoints);
    if (found == nullptr) {
      throw std::runtime_error("No spacepoint found for source-link index " +
                               std::to_string(index));
    }
    return found;
  };

  const auto s = track.size();
  if (s < 3) {
    throw std::runtime_error(
        "Cannot convert track with less then 3 spacepoints to seed");
  }

  std::vector<const SimSpacePoint*> ps;
  ps.reserve(track.size());

  std::transform(track.begin(), track.end(), std::back_inserter(ps),
                 findSpacePoint);
  std::ranges::sort(ps, {}, [](const auto& p) { return p->r(); });

  // Simply use r = m*z + t and solve for r=0 to find z vertex position...
  // Probably not the textbook way to do
  const auto m =
      (ps.back()->r() - ps.front()->r()) / (ps.back()->z() - ps.front()->z());
  const auto t = ps.front()->r() - m * ps.front()->z();
  const auto z_vertex = -t / m;

  SimSeed seed(*ps[0], *ps[s / 2], *ps[s - 1]);
  seed.setVertexZ(z_vertex);
  return seed;
}
