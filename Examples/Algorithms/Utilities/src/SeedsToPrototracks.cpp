// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
