// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Utilities/TrajectoriesToPrototracks.hpp"

#include "Acts/EventData/MultiTrajectory.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"

#include <utility>
#include <vector>

namespace ActsExamples {
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
