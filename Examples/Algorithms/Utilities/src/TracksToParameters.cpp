// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Utilities/TracksToParameters.hpp"

#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
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

TracksToParameters::TracksToParameters(Config cfg, Acts::Logging::Level lvl)
    : IAlgorithm("TracksToParameters", lvl), m_cfg(std::move(cfg)) {
  m_inputTracks.initialize(m_cfg.inputTracks);
  m_outputTrackParameters.initialize(m_cfg.outputTrackParameters);
}

ProcessCode TracksToParameters::execute(const AlgorithmContext& ctx) const {
  const auto& tracks = m_inputTracks(ctx);

  TrackParametersContainer trackParameters;
  trackParameters.reserve(tracks.size());

  for (const auto& track : tracks) {
    if (!track.hasReferenceSurface()) {
      ACTS_ERROR("Track has no reference surface");
    } else {
      trackParameters.push_back(track.createParametersAtReference());
    }
  }

  ACTS_INFO("Converted " << tracks.size() << " tracks to "
                         << trackParameters.size() << " track parameters");

  m_outputTrackParameters(ctx, std::move(trackParameters));

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
