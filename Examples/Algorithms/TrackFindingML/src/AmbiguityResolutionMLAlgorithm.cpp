// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TrackFindingML/AmbiguityResolutionMLAlgorithm.hpp"

#include "ActsExamples/Framework/ProcessCode.hpp"

#include <iterator>
#include <map>

ActsExamples::AmbiguityResolutionMLAlgorithm::AmbiguityResolutionMLAlgorithm(
    ActsExamples::AmbiguityResolutionMLAlgorithm::Config cfg,
    Acts::Logging::Level lvl)
    : ActsExamples::AmbiguityResolutionML("AmbiguityResolutionMLAlgorithm",
                                          lvl),
      m_cfg(std::move(cfg)),
      m_duplicateClassifier(m_cfg.inputDuplicateNN.c_str()) {
  if (m_cfg.inputTracks.empty()) {
    throw std::invalid_argument("Missing trajectories input collection");
  }
  if (m_cfg.outputTracks.empty()) {
    throw std::invalid_argument("Missing trajectories output collection");
  }
  m_inputTracks.initialize(m_cfg.inputTracks);
  m_outputTracks.initialize(m_cfg.outputTracks);
}

ActsExamples::ProcessCode ActsExamples::AmbiguityResolutionMLAlgorithm::execute(
    const AlgorithmContext& ctx) const {
  // Read input data
  const auto& tracks = m_inputTracks(ctx);
  // Associate measurement to their respective tracks
  std::multimap<int, std::pair<std::size_t, std::vector<std::size_t>>>
      trackMap = mapTrackHits(tracks, m_cfg.nMeasurementsMin);
  auto cluster = Acts::detail::clusterDuplicateTracks(trackMap);
  // Select the ID of the track we want to keep
  std::vector<std::size_t> goodTracks =
      m_duplicateClassifier.solveAmbiguity(cluster, tracks);
  // Prepare the output track collection from the IDs
  auto outputTracks = prepareOutputTrack(tracks, goodTracks);
  m_outputTracks(ctx, std::move(outputTracks));

  return ActsExamples::ProcessCode::SUCCESS;
}
