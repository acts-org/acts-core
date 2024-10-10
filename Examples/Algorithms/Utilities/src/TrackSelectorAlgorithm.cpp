// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Utilities/TrackSelectorAlgorithm.hpp"

#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackProxy.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "ActsExamples/EventData/Track.hpp"

#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>

namespace ActsExamples {
struct AlgorithmContext;
}  // namespace ActsExamples

ActsExamples::TrackSelectorAlgorithm::TrackSelectorAlgorithm(
    const Config& config, Acts::Logging::Level level)
    : IAlgorithm("TrackSelector", level),
      m_cfg(config),
      m_selector(config.selectorConfig) {
  if (m_cfg.inputTracks.empty()) {
    throw std::invalid_argument("Input track collection is empty");
  }

  if (m_cfg.outputTracks.empty()) {
    throw std::invalid_argument("Output track collection is empty");
  }

  m_inputTrackContainer.initialize(m_cfg.inputTracks);
  m_outputTrackContainer.initialize(m_cfg.outputTracks);
}

ActsExamples::ProcessCode ActsExamples::TrackSelectorAlgorithm::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  ACTS_VERBOSE("Reading tracks from: " << m_cfg.inputTracks);

  const auto& inputTracks = m_inputTrackContainer(ctx);

  std::shared_ptr<Acts::ConstVectorMultiTrajectory> trackStateContainer =
      inputTracks.trackStateContainerHolder();

  auto trackContainer = std::make_shared<Acts::VectorTrackContainer>();

  // temporary empty track state container: we don't change the original one,
  // but we need one for filtering
  auto tempTrackStateContainer =
      std::make_shared<Acts::VectorMultiTrajectory>();

  TrackContainer filteredTracks{trackContainer, tempTrackStateContainer};
  filteredTracks.ensureDynamicColumns(inputTracks);

  ACTS_DEBUG("Track container size before filtering: " << inputTracks.size());

  m_selector.selectTracks(inputTracks, filteredTracks);

  ACTS_DEBUG("Track container size after filtering: " << filteredTracks.size());

  ConstTrackContainer outputTracks{
      std::make_shared<Acts::ConstVectorTrackContainer>(
          std::move(*trackContainer)),
      trackStateContainer};

  m_outputTrackContainer(ctx, std::move(outputTracks));

  return ProcessCode::SUCCESS;
}
