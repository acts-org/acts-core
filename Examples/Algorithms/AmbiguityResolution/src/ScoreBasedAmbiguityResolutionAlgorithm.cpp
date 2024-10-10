// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/AmbiguityResolution/ScoreBasedAmbiguityResolutionAlgorithm.hpp"

#include "Acts/AmbiguityResolution/ScoreBasedAmbiguityResolution.hpp"
#include "Acts/EventData/MultiTrajectoryHelpers.hpp"
#include "Acts/Plugins/Json/AmbiguityConfigJsonConverter.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <fstream>

namespace {

Acts::ScoreBasedAmbiguityResolution::Config transformConfig(
    const ActsExamples::ScoreBasedAmbiguityResolutionAlgorithm::Config& cfg,
    const std::string& configFile) {
  Acts::ScoreBasedAmbiguityResolution::Config result;

  Acts::ConfigPair configPair;
  nlohmann::json json_file;
  std::ifstream file(configFile);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << configFile << std::endl;
    return {};
  }
  file >> json_file;
  file.close();

  Acts::from_json(json_file, configPair);

  result.volumeMap = configPair.first;
  result.detectorConfigs = configPair.second;
  result.minScore = cfg.minScore;
  result.minScoreSharedTracks = cfg.minScoreSharedTracks;
  result.maxSharedTracksPerMeasurement = cfg.maxSharedTracksPerMeasurement;
  result.maxShared = cfg.maxShared;
  result.pTMin = cfg.pTMin;
  result.pTMax = cfg.pTMax;
  result.phiMin = cfg.phiMin;
  result.phiMax = cfg.phiMax;
  result.etaMin = cfg.etaMin;
  result.etaMax = cfg.etaMax;
  result.useAmbiguityFunction = cfg.useAmbiguityFunction;
  return result;
}

std::size_t sourceLinkHash(const Acts::SourceLink& a) {
  return static_cast<std::size_t>(
      a.get<ActsExamples::IndexSourceLink>().index());
}

bool sourceLinkEquality(const Acts::SourceLink& a, const Acts::SourceLink& b) {
  return a.get<ActsExamples::IndexSourceLink>().index() ==
         b.get<ActsExamples::IndexSourceLink>().index();
}

bool doubleHolesFilter(const Acts::TrackProxy<Acts::ConstVectorTrackContainer,
                                              Acts::ConstVectorMultiTrajectory,
                                              std::shared_ptr, true>& track) {
  bool doubleFlag = false;
  int counter = 0;
  for (const auto& ts : track.trackStatesReversed()) {
    auto iTypeFlags = ts.typeFlags();
    if (!iTypeFlags.test(Acts::TrackStateFlag::HoleFlag)) {
      doubleFlag = false;
    }

    if (iTypeFlags.test(Acts::TrackStateFlag::HoleFlag)) {
      if (doubleFlag) {
        counter++;
        doubleFlag = false;
      } else {
        doubleFlag = true;
      };
    }
  }
  if (counter > 1) {
    return true;
  } else {
    return false;
  }
}

}  // namespace

ActsExamples::ScoreBasedAmbiguityResolutionAlgorithm::
    ScoreBasedAmbiguityResolutionAlgorithm(
        ActsExamples::ScoreBasedAmbiguityResolutionAlgorithm::Config cfg,
        Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("ScoreBasedAmbiguityResolutionAlgorithm", lvl),
      m_cfg(std::move(cfg)),
      m_ambi(transformConfig(cfg, m_cfg.configFile), logger().clone()) {
  if (m_cfg.inputTracks.empty()) {
    throw std::invalid_argument("Missing trajectories input collection");
  }
  if (m_cfg.outputTracks.empty()) {
    throw std::invalid_argument("Missing trajectories output collection");
  }
  m_inputTracks.initialize(m_cfg.inputTracks);
  m_outputTracks.initialize(m_cfg.outputTracks);
}

ActsExamples::ProcessCode
ActsExamples::ScoreBasedAmbiguityResolutionAlgorithm::execute(
    const AlgorithmContext& ctx) const {
  const auto& tracks = m_inputTracks(ctx);  // Read input data
  ACTS_VERBOSE("Number of input tracks: " << tracks.size());

  std::vector<std::vector<Acts::ScoreBasedAmbiguityResolution::MeasurementInfo>>
      measurementsPerTracks;

  std::vector<std::vector<Acts::ScoreBasedAmbiguityResolution::TrackFeatures>>
      trackFeaturesVectors;
  measurementsPerTracks = m_ambi.computeInitialState(
      tracks, &sourceLinkHash, &sourceLinkEquality, trackFeaturesVectors);

  Acts::ScoreBasedAmbiguityResolution::OptionalCuts<ConstTrackProxy>
      optionalCuts;
  optionalCuts.cuts.push_back(doubleHolesFilter);
  std::vector<int> goodTracks = m_ambi.solveAmbiguity(
      tracks, measurementsPerTracks, trackFeaturesVectors, optionalCuts);
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
