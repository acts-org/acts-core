// This file is part of the Acts project.
//
// Copyright (C) 2020 Acts project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvMultiTrajectoryWriter.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "Acts/EventData/MultiTrajectoryHelpers.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"

#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace ActsExamples;

CsvMultiTrajectoryWriter::CsvMultiTrajectoryWriter(const CsvMultiTrajectoryWriter::Config& cfg,
						   Acts::Logging::Level level)
  : WriterT<TrajectoriesContainer>(cfg.inputTrajectories,
				   "CsvMultiTrajectoryWriter", level),
    m_cfg(cfg) 
{
  if (m_cfg.inputTrajectories.empty()) {
    throw std::invalid_argument("Missing input trajectories collection");
  }
}

ProcessCode CsvMultiTrajectoryWriter::writeT(const AlgorithmContext& context,
					     const TrajectoriesContainer& trajectories) {
  // open per-event file
  std::string path = perEventFilepath(m_cfg.outputDir, "CKFtracks.csv",
                                          context.eventNumber);
  std::ofstream mos(path, std::ofstream::out | std::ofstream::trunc);
  if (!mos) {
    throw std::ios_base::failure("Could not open '" + path + "' to write");
  }

  using HitParticlesMap = ActsExamples::IndexMultimap<ActsFatras::Barcode>;
  const auto& hitParticlesMap =
    context.eventStore.get<HitParticlesMap>(m_cfg.inputMeasurementParticlesMap);

  struct trackInfo {
    size_t track_id;
    size_t nMeasurements;
    size_t nOutliers;
    size_t nHoles;
    double chi2Sum;
    size_t NDF;
    size_t nMajorityHits;
    size_t trackType;
    double truthMatchProb;
    const TrackParameters* fitterParameters;
  };
  std::unordered_map<size_t,trackInfo> infoMap;
  
  // Counter of truth-matched reco tracks
  using RecoTrackInfo = std::pair< trackInfo, size_t>; 
  std::map<ActsFatras::Barcode, std::vector<RecoTrackInfo>> matched;



  size_t track_id = 0;
  for (const auto& traj : trajectories) {
    // The trajectory entry indices and the multiTrajectory
    const auto& trackTips = traj.tips();
    const auto& mj = traj.multiTrajectory();
    if (trackTips.empty()) {
      ACTS_WARNING("Empty multiTrajectory.");
      continue;
    }

    // Loop over all trajectories in a multiTrajectory
    for (const size_t& trackTip : trackTips) {
      // Collect the trajectory summary info
      auto trajState =
	Acts::MultiTrajectoryHelpers::trajectoryState(mj, trackTip);
      // Reco track selection
      //@TODO: add interface for applying others cuts on reco tracks:
      // -> pT, d0, z0, detector-specific hits/holes number cut
      if (trajState.nMeasurements < m_cfg.nMeasurementsMin) {
        continue;
      }

      // Check if the reco track has fitted track parameters
      if (not traj.hasTrackParameters(trackTip)) {
        ACTS_WARNING(
            "No fitted track parameters for trajectory with entry index = "
            << trackTip);
        continue;
      }

      // Get the majority truth particle to this track
      std::vector<ParticleHitCount> particleHitCount;
      identifyContributingParticles(hitParticlesMap,traj,trackTip,
				    particleHitCount);
      if (particleHitCount.empty()) {
        ACTS_WARNING(
            "No truth particle associated with this trajectory with entry "
            "index = "
            << trackTip);
        continue;
      }

      // Get the majority particle counts
      ActsFatras::Barcode majorityParticleId =
	particleHitCount.front().particleId;
      // n Majority hits
      size_t nMajorityHits = particleHitCount.front().hitCount;

      // track info
      trackInfo toAdd;
      toAdd.track_id = track_id;
      toAdd.nMajorityHits = nMajorityHits;
      toAdd.nMeasurements = trajState.nMeasurements;
      toAdd.nOutliers = trajState.nOutliers;
      toAdd.nHoles = trajState.nHoles;
      toAdd.chi2Sum = trajState.chi2Sum * 1.;
      toAdd.NDF = trajState.NDF;
      toAdd.truthMatchProb = toAdd.nMajorityHits * 1. / trajState.nMeasurements;
      toAdd.fitterParameters = &traj.trackParameters(trackTip);
      toAdd.trackType = -1;

      // Requirement on the pT of the track
      const auto& momentum = toAdd.fitterParameters->momentum();
      const auto pT = Acts::VectorHelpers::perp(momentum);
      if (pT < m_cfg.ptMin) {
        continue;
      }

      // Check if the trajectory is matched with truth.
      if ( toAdd.truthMatchProb >= m_cfg.truthMatchProbMin) {
        matched[majorityParticleId].push_back( {toAdd, track_id} ); 
      }

      infoMap[ track_id ] = toAdd;

      track_id++;
    }  // end of one trajectory
  }    // end of multi-trajectory





  // Find duplicates
  std::unordered_set< size_t > listDuplicates;
  for (auto& [particleId, matchedTracks] : matched) {   

    std::sort(matchedTracks.begin(), matchedTracks.end(), 
	      [](const RecoTrackInfo& lhs, const RecoTrackInfo& rhs) {
	        return lhs.first.nMajorityHits > rhs.first.nMajorityHits;
	      }); 

    for (size_t itrack = 0; itrack < matchedTracks.size(); itrack++) {
      const auto& [Hits, trackID] = matchedTracks.at(itrack);   

      if (itrack != 0)
	listDuplicates.insert( trackID );
    }

  }




  // write csv header
  mos << "track_id, nMajorityHits, nMeasurements, nOutliers, nHoles, chi2, ndf, chi2/ndf, pT,"
    "truthMatchProbability, good/duplicate/fake";
  mos << '\n';
  mos << std::setprecision(m_cfg.outputPrecision);
  
  // good/duplicate/fake = 0/1/2
  for (auto key : infoMap) {
    auto ID = key.first;
    auto trajState = key.second;

    if (trajState.truthMatchProb < m_cfg.truthMatchProbMin) {
      trajState.trackType = 2;
    } else if (listDuplicates.find(ID) != listDuplicates.end()) {
      trajState.trackType = 1;
    } else { 
      trajState.trackType = 0;
    }
    
    // write the track info
    mos << trajState.track_id << ",";
    mos << trajState.nMajorityHits << ","; 
    mos << trajState.nMeasurements << ",";
    mos << trajState.nOutliers << ",";
    mos << trajState.nHoles << ",";
    mos << trajState.chi2Sum * 1.0 << ",";
    mos << trajState.NDF << ",";
    mos << trajState.chi2Sum * 1.0 / trajState.NDF << ",";
    mos << Acts::VectorHelpers::perp(trajState.fitterParameters->momentum()) << ",";
    mos << trajState.truthMatchProb << ",";
    mos << trajState.trackType ;
    mos << '\n';
  }


  return ProcessCode::SUCCESS;
}
