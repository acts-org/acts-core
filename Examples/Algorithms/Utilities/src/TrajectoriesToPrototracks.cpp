// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Utilities/TrajectoriesToPrototracks.hpp"

#include "Acts/EventData/MultiTrajectory.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"

#include <utility>
#include <vector>

namespace ActsExamples {
class IndexSourceLink;
struct AlgorithmContext;

TrajectoriesToPrototracks::TrajectoriesToPrototracks(Config cfg,
                                                     Acts::Logging::Level lvl)
    : IAlgorithm("TrajectoriesToPrototracks", lvl), m_cfg(std::move(cfg)) {
  m_inputTrajectories.initialize(m_cfg.inputTrajectories);
  m_outputProtoTracks.initialize(m_cfg.outputProtoTracks);
}

ProcessCode TrajectoriesToPrototracks::execute(
    const AlgorithmContext& ctx) const {
  const auto trajectories = m_inputTrajectories(ctx);

  ProtoTrackContainer tracks;

  for (const auto& trajectory : trajectories) {
    for (const auto tip : trajectory.tips()) {
      ProtoTrack track;

      trajectory.multiTrajectory().visitBackwards(tip, [&](const auto& state) {
        if (!state.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
          return true;
        }

        const auto sourceLink =
            state.getUncalibratedSourceLink().template get<IndexSourceLink>();
        track.push_back(sourceLink.index());

        return true;
      });

      tracks.push_back(track);
    }
  }

  m_outputProtoTracks(ctx, std::move(tracks));

  return ProcessCode::SUCCESS;
}
}  // namespace ActsExamples
