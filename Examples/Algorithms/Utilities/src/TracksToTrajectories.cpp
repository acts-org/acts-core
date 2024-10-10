// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Utilities/TracksToTrajectories.hpp"

#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/ProxyAccessor.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackProxy.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/HashedString.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace ActsExamples {
struct AlgorithmContext;

TracksToTrajectories::TracksToTrajectories(Config cfg, Acts::Logging::Level lvl)
    : IAlgorithm("TracksToTrajectories", lvl), m_cfg(std::move(cfg)) {
  m_inputTracks.initialize(m_cfg.inputTracks);
  m_outputTrajectories.initialize(m_cfg.outputTrajectories);
}

ProcessCode TracksToTrajectories::execute(const AlgorithmContext& ctx) const {
  const auto& tracks = m_inputTracks(ctx);

  // Prepare the output data with MultiTrajectory
  TrajectoriesContainer trajectories;
  trajectories.reserve(tracks.size());

  static const Acts::ConstProxyAccessor<unsigned int> seedNumber("trackGroup");

  if (tracks.hasColumn(Acts::hashString("trackGroup"))) {
    // track group by seed is available, produce grouped trajectories
    std::optional<unsigned int> lastSeed;

    Trajectories::IndexedParameters parameters;
    std::vector<Acts::MultiTrajectoryTraits::IndexType> tips;

    for (const auto& track : tracks) {
      if (!lastSeed) {
        lastSeed = seedNumber(track);
      }

      if (seedNumber(track) != lastSeed.value()) {
        // make copies and clear vectors
        trajectories.emplace_back(tracks.trackStateContainer(), tips,
                                  parameters);
        tips.clear();
        parameters.clear();
      }

      lastSeed = seedNumber(track);

      tips.push_back(track.tipIndex());
      parameters.emplace(
          std::pair{track.tipIndex(),
                    TrackParameters{track.referenceSurface().getSharedPtr(),
                                    track.parameters(), track.covariance(),
                                    track.particleHypothesis()}});
    }

    if (tips.empty()) {
      ACTS_DEBUG("Last trajectory is empty");
    }

    // last entry: move vectors
    trajectories.emplace_back(tracks.trackStateContainer(), std::move(tips),
                              std::move(parameters));

  } else {
    // no grouping by seed, make one trajectory per track

    for (const auto& track : tracks) {
      if (!track.hasReferenceSurface()) {
        ACTS_WARNING("Unable to convert track with tip "
                     << track.tipIndex()
                     << " because no reference surface is set");
        continue;
      }
      Trajectories::IndexedParameters parameters;
      parameters.reserve(1);
      std::vector<Acts::MultiTrajectoryTraits::IndexType> tips;
      tips.reserve(1);

      tips.push_back(track.tipIndex());
      parameters.emplace(
          std::pair{track.tipIndex(),
                    TrackParameters{track.referenceSurface().getSharedPtr(),
                                    track.parameters(), track.covariance(),
                                    track.particleHypothesis()}});

      trajectories.emplace_back(tracks.trackStateContainer(), std::move(tips),
                                std::move(parameters));
    }
  }

  m_outputTrajectories(ctx, std::move(trajectories));

  return ProcessCode::SUCCESS;
}
}  // namespace ActsExamples
