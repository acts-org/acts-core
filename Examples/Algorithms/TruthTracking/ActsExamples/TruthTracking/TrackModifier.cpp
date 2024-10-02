// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TruthTracking/TrackModifier.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "ActsExamples/EventData/Track.hpp"

#include <stdexcept>
#include <utility>

namespace ActsExamples {

TrackModifier::TrackModifier(const Config& config, Acts::Logging::Level level)
    : IAlgorithm("TrackModifier", level), m_cfg(config) {
  if (m_cfg.inputTracks.empty()) {
    throw std::invalid_argument("Missing input tracks");
  }
  if (m_cfg.outputTracks.empty()) {
    throw std::invalid_argument("Missing output tracks");
  }

  m_inputTracks.initialize(m_cfg.inputTracks);
  m_outputTracks.initialize(m_cfg.outputTracks);
}

ProcessCode TrackModifier::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  auto modifyTrack = [this](auto& trk) {
    {
      if (m_cfg.killTime) {
        trk.parameters()[Acts::eBoundTime] = 0;
      }
    }

    {
      if (m_cfg.dropCovariance) {
        trk.covariance() =
            Acts::BoundSquareMatrix(trk.covariance().diagonal().asDiagonal());
      }
      if (m_cfg.covScale != 1) {
        trk.covariance() *= m_cfg.covScale;
      }
      if (m_cfg.killTime) {
        trk.covariance().row(Acts::eBoundTime).setZero();
        trk.covariance().col(Acts::eBoundTime).setZero();
        trk.covariance()(Acts::eBoundTime, Acts::eBoundTime) = 1;
      }
    }

    return trk;
  };

  const auto& inputTracks = m_inputTracks(ctx);

  auto trackContainer = std::make_shared<Acts::VectorTrackContainer>();
  auto trackStateContainer = std::make_shared<Acts::VectorMultiTrajectory>();
  TrackContainer outputTracks(trackContainer, trackStateContainer);
  outputTracks.ensureDynamicColumns(inputTracks);

  for (const auto& inputTrack : inputTracks) {
    auto outputTrack = outputTracks.makeTrack();
    outputTrack.copyFrom(inputTrack);
    modifyTrack(outputTrack);
  }

  auto constTrackStateContainer =
      std::make_shared<Acts::ConstVectorMultiTrajectory>(
          std::move(*trackStateContainer));
  auto constTrackContainer = std::make_shared<Acts::ConstVectorTrackContainer>(
      std::move(*trackContainer));
  ConstTrackContainer constTracks{constTrackContainer,
                                  constTrackStateContainer};

  m_outputTracks(ctx, std::move(constTracks));

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
