// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TrackFindingML/AmbiguityResolutionMLAlgorithm.hpp"

#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <iterator>
#include <map>

static std::size_t sourceLinkHash(const Acts::SourceLink& a) {
  return static_cast<std::size_t>(
      a.get<ActsExamples::IndexSourceLink>().index());
}

static bool sourceLinkEquality(const Acts::SourceLink& a,
                               const Acts::SourceLink& b) {
  return a.get<ActsExamples::IndexSourceLink>().index() ==
         b.get<ActsExamples::IndexSourceLink>().index();
}

ActsExamples::AmbiguityResolutionMLAlgorithm::AmbiguityResolutionMLAlgorithm(
    ActsExamples::AmbiguityResolutionMLAlgorithm::Config cfg,
    Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("AmbiguityResolutionMLAlgorithm", lvl),
      m_cfg(std::move(cfg)),
      m_ambiML(m_cfg.toAmbiguityResolutionMLConfig(), logger().clone()) {
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
  // Associate measurement to their respective tracks to prepare the track
  // shared hits based clustering
  std::multimap<int, std::pair<std::size_t, std::vector<std::size_t>>>
      trackMap =
          m_ambiML.mapTrackHits(tracks, &sourceLinkHash, &sourceLinkEquality);
  // Cluster the tracks based on the shared hits
  auto cluster = Acts::detail::clusterDuplicateTracks(trackMap);
  // Select the ID of the track we want to keep
  std::vector<std::size_t> goodTracks =
      m_ambiML.solveAmbiguity(cluster, tracks);
  // Prepare the output track collection from the IDs
  TrackContainer solvedTracks{std::make_shared<Acts::VectorTrackContainer>(),
                              std::make_shared<Acts::VectorMultiTrajectory>()};
  solvedTracks.ensureDynamicColumns(tracks);
  for (auto iTrack : goodTracks) {
    auto destProxy = solvedTracks.makeTrack();
    auto srcProxy = tracks.getTrack(iTrack);
    destProxy.copyFrom(srcProxy, false);
    destProxy.tipIndex() = srcProxy.tipIndex();
  }

  ActsExamples::ConstTrackContainer outputTracks{
      std::make_shared<Acts::ConstVectorTrackContainer>(
          std::move(solvedTracks.container())),
      tracks.trackStateContainerHolder()};

  m_outputTracks(ctx, std::move(outputTracks));

  return ActsExamples::ProcessCode::SUCCESS;
}
