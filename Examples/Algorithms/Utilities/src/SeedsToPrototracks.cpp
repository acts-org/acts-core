// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Utilities/SeedsToPrototracks.hpp"

#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/Utilities/EventDataTransforms.hpp"

#include <utility>

namespace ActsExamples {
struct AlgorithmContext;

SeedsToPrototracks::SeedsToPrototracks(Config cfg, Acts::Logging::Level lvl)
    : IAlgorithm("SeedsToPrototracks", lvl), m_cfg(std::move(cfg)) {
  m_inputSeeds.initialize(m_cfg.inputSeeds);
  m_outputProtoTracks.initialize(m_cfg.outputProtoTracks);
}

ProcessCode SeedsToPrototracks::execute(const AlgorithmContext& ctx) const {
  const auto seeds = m_inputSeeds(ctx);

  ProtoTrackContainer tracks;
  tracks.reserve(seeds.size());

  std::transform(seeds.begin(), seeds.end(), std::back_inserter(tracks),
                 seedToPrototrack);

  m_outputProtoTracks(ctx, std::move(tracks));

  return ProcessCode::SUCCESS;
}
}  // namespace ActsExamples
