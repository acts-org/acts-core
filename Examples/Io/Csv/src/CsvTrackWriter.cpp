// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/Csv/CsvTrackWriter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/ProxyAccessor.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/MultiIndex.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsExamples/Utilities/Range.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

using namespace ActsExamples;

CsvTrackWriter::CsvTrackWriter(const CsvTrackWriter::Config& config,
                               Acts::Logging::Level level)
    : WriterT<ConstTrackContainer>(config.inputTracks, "CsvTrackWriter", level),
      m_cfg(config) {
  if (m_cfg.inputTracks.empty()) {
    throw std::invalid_argument("Missing input tracks collection");
  }

  m_inputMeasurementParticlesMap.initialize(m_cfg.inputMeasurementParticlesMap);
}

ProcessCode CsvTrackWriter::writeT(const AlgorithmContext& context,
                                   const ConstTrackContainer& tracks) {
  // open per-event file
  std::string path =
      perEventFilepath(m_cfg.outputDir, m_cfg.fileName, context.eventNumber);
  std::ofstream mos(path, std::ofstream::out | std::ofstream::trunc);
  if (!mos) {
    throw std::ios_base::failure("Could not open '" + path + "' to write");
  }

  const auto& hitParticlesMap = m_inputMeasurementParticlesMap(context);

  std::unordered_map<Acts::MultiTrajectoryTraits::IndexType, TrackInfo> infoMap;

  // Counter of truth-matched reco tracks
  using RecoTrackInfo = std::pair<TrackInfo, std::size_t>;
  std::map<ActsFatras::Barcode, std::vector<RecoTrackInfo>> matched;

  std::size_t trackId = 0;
  for (const auto& track : tracks) {
    // Reco track selection
    //@TODO: add interface for applying others cuts on reco tracks:
    // -> pT, d0, z0, detector-specific hits/holes number cut
    if (track.nMeasurements() < m_cfg.nMeasurementsMin) {
      continue;
    }

    // Check if the reco track has fitted track parameters
    if (!track.hasReferenceSurface()) {
      ACTS_WARNING(
          "No fitted track parameters for trajectory with entry index = "
          << track.tipIndex());
      continue;
    }

    // Get the majority truth particle to this track
    std::vector<ParticleHitCount> particleHitCount;
    identifyContributingParticles(hitParticlesMap, track, particleHitCount);
    if (m_cfg.onlyTruthMatched && particleHitCount.empty()) {
      ACTS_WARNING(
          "No truth particle associated with this trajectory with entry "
          "index = "
          << track.tipIndex());
      continue;
    }

    // Requirement on the pT of the track
    auto params = track.createParametersAtReference();
    const auto momentum = params.momentum();
    const auto pT = Acts::VectorHelpers::perp(momentum);
    if (pT < m_cfg.ptMin) {
      continue;
    }
    std::size_t nMajorityHits = 0;
    ActsFatras::Barcode majorityParticleId;
    if (!particleHitCount.empty()) {
      // Get the majority particle counts
      majorityParticleId = particleHitCount.front().particleId;
      // n Majority hits
      nMajorityHits = particleHitCount.front().hitCount;
    }

    static const Acts::ConstProxyAccessor<unsigned int> seedNumber(
        "trackGroup");

    // track info
    TrackInfo toAdd;
    toAdd.trackId = trackId;
    if (tracks.hasColumn(Acts::hashString("trackGroup"))) {
      toAdd.seedID = seedNumber(track);
    } else {
      toAdd.seedID = 0;
    }
    toAdd.particleId = majorityParticleId;
    toAdd.nStates = track.nTrackStates();
    toAdd.nMajorityHits = nMajorityHits;
    toAdd.nMeasurements = track.nMeasurements();
    toAdd.nOutliers = track.nOutliers();
    toAdd.nHoles = track.nHoles();
    toAdd.nSharedHits = track.nSharedHits();
    toAdd.chi2Sum = track.chi2();
    toAdd.NDF = track.nDoF();
    toAdd.truthMatchProb = toAdd.nMajorityHits * 1. / track.nMeasurements();
    toAdd.fittedParameters = params;
    toAdd.trackType = "unknown";

    for (const auto& state : track.trackStatesReversed()) {
      if (state.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
        auto sl =
            state.getUncalibratedSourceLink().template get<IndexSourceLink>();
        auto hitIndex = sl.index();
        toAdd.measurementsID.insert(toAdd.measurementsID.begin(), hitIndex);
      }
    }

    // Check if the trajectory is matched with truth.
    if (toAdd.truthMatchProb >= m_cfg.truthMatchProbMin) {
      matched[toAdd.particleId].push_back({toAdd, toAdd.trackId});
    } else {
      toAdd.trackType = "fake";
    }

    infoMap[toAdd.trackId] = toAdd;

    trackId++;
  }

  // Find duplicates
  std::unordered_set<std::size_t> listGoodTracks;
  for (auto& [particleId, matchedTracks] : matched) {
    std::ranges::sort(matchedTracks, [](const auto& lhs, const auto& rhs) {
      const auto& t1 = lhs.first;
      const auto& t2 = rhs.first;
      // nMajorityHits are sorted descending, others ascending
      return std::tie(t2.nMajorityHits, t1.nOutliers, t1.chi2Sum) <
             std::tie(t1.nMajorityHits, t2.nOutliers, t2.chi2Sum);
    });

    listGoodTracks.insert(matchedTracks.front().first.trackId);
  }

  // write csv header
  mos << "track_id,seed_id,particleId,"
      << "nStates,nMajorityHits,nMeasurements,nOutliers,nHoles,nSharedHits,"
      << "chi2,ndf,chi2/ndf,"
      << "pT,eta,phi,"
      << "truthMatchProbability,"
      << "good/duplicate/fake,"
      << "Hits_ID";

  mos << '\n';
  mos << std::setprecision(m_cfg.outputPrecision);

  // good/duplicate/fake = 0/1/2
  for (auto& [id, trajState] : infoMap) {
    if (listGoodTracks.contains(id)) {
      trajState.trackType = "good";
    } else if (trajState.trackType != "fake") {
      trajState.trackType = "duplicate";
    }

    const auto& params = *trajState.fittedParameters;
    const auto momentum = params.momentum();

    // write the track info
    mos << trajState.trackId << ",";
    mos << trajState.seedID << ",";
    mos << trajState.particleId << ",";
    mos << trajState.nStates << ",";
    mos << trajState.nMajorityHits << ",";
    mos << trajState.nMeasurements << ",";
    mos << trajState.nOutliers << ",";
    mos << trajState.nHoles << ",";
    mos << trajState.nSharedHits << ",";
    mos << trajState.chi2Sum << ",";
    mos << trajState.NDF << ",";
    mos << trajState.chi2Sum * 1.0 / trajState.NDF << ",";
    mos << Acts::VectorHelpers::perp(momentum) << ",";
    mos << Acts::VectorHelpers::eta(momentum) << ",";
    mos << Acts::VectorHelpers::phi(momentum) << ",";
    mos << trajState.truthMatchProb << ",";
    mos << trajState.trackType << ",";
    mos << "\"[";
    for (auto& ID : trajState.measurementsID) {
      mos << ID << ",";
    }
    mos << "]\"";
    mos << '\n';
  }

  return ProcessCode::SUCCESS;
}
