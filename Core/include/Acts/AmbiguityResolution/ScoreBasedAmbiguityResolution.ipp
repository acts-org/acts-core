// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/AmbiguityResolution/ScoreBasedAmbiguityResolution.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TrackContainerFrontendConcept.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <unordered_map>

namespace Acts {

inline const Logger& ScoreBasedAmbiguityResolution::logger() const {
  return *m_logger;
}

template <TrackContainerFrontend track_container_t, typename source_link_hash_t,
          typename source_link_equality_t>
std::vector<std::vector<ScoreBasedAmbiguityResolution::MeasurementInfo>>
ScoreBasedAmbiguityResolution::computeInitialState(
    const track_container_t& tracks, source_link_hash_t sourceLinkHash,
    source_link_equality_t sourceLinkEquality,
    std::vector<std::vector<TrackFeatures>>& trackFeaturesVectors) const {
  auto MeasurementIndexMap =
      std::unordered_map<SourceLink, std::size_t, source_link_hash_t,
                         source_link_equality_t>(0, sourceLinkHash,
                                                 sourceLinkEquality);

  std::vector<std::vector<MeasurementInfo>> measurementsPerTrack;
  measurementsPerTrack.reserve(tracks.size());
  ACTS_VERBOSE("Starting to compute initial state");

  for (const auto& track : tracks) {
    int numberOfDetectors = m_cfg.detectorConfigs.size();
    int numberOfTrackStates = track.nTrackStates();
    std::vector<MeasurementInfo> measurements;
    measurements.reserve(numberOfTrackStates);
    std::vector<TrackFeatures> trackFeaturesVector(numberOfDetectors);

    for (const auto& ts : track.trackStatesReversed()) {
      if (!ts.hasReferenceSurface()) {
        ACTS_DEBUG("Track state has no reference surface");
        continue;
      }
      auto iVolume = ts.referenceSurface().geometryId().volume();
      auto volume_it = m_cfg.volumeMap.find(iVolume);
      if (volume_it == m_cfg.volumeMap.end()) {
        ACTS_ERROR("Volume " << iVolume << "not found in the volume map");
        continue;
      }
      auto detectorId = volume_it->second;

      if (ts.typeFlags().test(Acts::TrackStateFlag::HoleFlag)) {
        ACTS_DEBUG("Track state type is HoleFlag");
        trackFeaturesVector[detectorId].nHoles++;
      } else if (ts.typeFlags().test(Acts::TrackStateFlag::OutlierFlag)) {
        Acts::SourceLink sourceLink = ts.getUncalibratedSourceLink();
        ACTS_DEBUG("Track state type is OutlierFlag");
        trackFeaturesVector[detectorId].nOutliers++;

        // assign a new measurement index if the source link was not seen yet
        auto emplace = MeasurementIndexMap.try_emplace(
            sourceLink, MeasurementIndexMap.size());

        bool isOutliner = true;

        measurements.push_back({emplace.first->second, detectorId, isOutliner});
      } else if (ts.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
        Acts::SourceLink sourceLink = ts.getUncalibratedSourceLink();
        ACTS_DEBUG("Track state type is MeasurementFlag");

        if (ts.typeFlags().test(Acts::TrackStateFlag::SharedHitFlag)) {
          trackFeaturesVector[detectorId].nSharedHits++;
        }
        trackFeaturesVector[detectorId].nHits++;

        // assign a new measurement index if the source link was not seen yet
        auto emplace = MeasurementIndexMap.try_emplace(
            sourceLink, MeasurementIndexMap.size());

        bool isoutliner = false;

        measurements.push_back({emplace.first->second, detectorId, isoutliner});
      }
    }
    measurementsPerTrack.push_back(std::move(measurements));
    trackFeaturesVectors.push_back(std::move(trackFeaturesVector));
  }

  return measurementsPerTrack;
}

template <TrackContainerFrontend track_container_t>
std::vector<double> Acts::ScoreBasedAmbiguityResolution::simpleScore(
    const track_container_t& tracks,
    const std::vector<std::vector<TrackFeatures>>& trackFeaturesVectors,
    const OptionalCuts<typename track_container_t::ConstTrackProxy>&
        optionalCuts) const {
  std::vector<double> trackScore;
  trackScore.reserve(tracks.size());

  int iTrack = 0;

  ACTS_VERBOSE("Number of detectors: " << m_cfg.detectorConfigs.size());

  ACTS_INFO("Starting to score tracks");

  // Loop over all the tracks in the container
  for (const auto& track : tracks) {
    // get the trackFeatures map for the track
    const auto& trackFeaturesVector = trackFeaturesVectors[iTrack];
    double score = 1;
    auto pT = Acts::VectorHelpers::perp(track.momentum());
    auto eta = Acts::VectorHelpers::eta(track.momentum());
    auto phi = Acts::VectorHelpers::phi(track.momentum());
    // cuts on pT
    if (pT < m_cfg.pTMin || pT > m_cfg.pTMax) {
      score = 0;
      iTrack++;
      trackScore.push_back(score);
      ACTS_DEBUG("Track: " << iTrack
                           << " has score = 0, due to pT cuts --- pT = " << pT);
      continue;
    }

    // cuts on phi
    if (phi > m_cfg.phiMax || phi < m_cfg.phiMin) {
      score = 0;
      iTrack++;
      trackScore.push_back(score);
      ACTS_DEBUG("Track: " << iTrack
                           << " has score = 0, due to phi cuts --- phi =  "
                           << phi);
      continue;
    }

    // cuts on eta
    if (eta > m_cfg.etaMax || eta < m_cfg.etaMin) {
      score = 0;
      iTrack++;
      trackScore.push_back(score);
      ACTS_DEBUG("Track: " << iTrack
                           << " has score = 0, due to eta cuts --- eta =  "
                           << eta);
      continue;
    }

    // cuts on optional cuts
    for (const auto& cutFunction : optionalCuts.cuts) {
      if (cutFunction(track)) {
        score = 0;
        ACTS_DEBUG("Track: " << iTrack
                             << " has score = 0, due to optional cuts.");
        break;
      }
    }

    if (score == 0) {
      iTrack++;
      trackScore.push_back(score);
      ACTS_DEBUG("Track: " << iTrack << " score : " << score);
      continue;
    }
    // Reject tracks which didn't pass the detector cuts.
    for (std::size_t detectorId = 0; detectorId < m_cfg.detectorConfigs.size();
         detectorId++) {
      const auto& detector = m_cfg.detectorConfigs.at(detectorId);

      const auto& trackFeatures = trackFeaturesVector[detectorId];

      ACTS_DEBUG("---> Found summary information");
      ACTS_DEBUG("---> Detector ID: " << detectorId);
      ACTS_DEBUG("---> Number of hits: " << trackFeatures.nHits);
      ACTS_DEBUG("---> Number of holes: " << trackFeatures.nHoles);
      ACTS_DEBUG("---> Number of outliers: " << trackFeatures.nOutliers);

      if ((trackFeatures.nHits < detector.minHits) ||
          (trackFeatures.nHoles > detector.maxHoles) ||
          (trackFeatures.nOutliers > detector.maxOutliers)) {
        score = 0;
        ACTS_DEBUG("Track: " << iTrack
                             << " has score = 0, due to detector cuts");
        break;
      }
    }

    if (score == 0) {
      iTrack++;
      trackScore.push_back(score);
      ACTS_DEBUG("Track: " << iTrack << " score : " << score);
      continue;
    }

    // real scoring starts here
    // if the ambiguity scoring function is used, the score is processed with a
    // different algorithm than the simple score.

    ACTS_VERBOSE("Using Simple Scoring function");

    score = 100;
    // Adding the score for each detector.
    // detector score is determined by the number of hits/hole/outliers *
    // hit/hole/outlier scoreWeights in a detector.
    for (std::size_t detectorId = 0; detectorId < m_cfg.detectorConfigs.size();
         detectorId++) {
      const auto& detector = m_cfg.detectorConfigs.at(detectorId);
      const auto& trackFeatures = trackFeaturesVector[detectorId];

      score += trackFeatures.nHits * detector.hitsScoreWeight;
      score += trackFeatures.nHoles * detector.holesScoreWeight;
      score += trackFeatures.nOutliers * detector.outliersScoreWeight;
      score += trackFeatures.nSharedHits * detector.otherScoreWeight;
    }

    // Adding scores based on optional weights
    for (const auto& weightFunction : optionalCuts.weights) {
      weightFunction(track, score);
    }

    // Adding the score based on the chi2/ndf
    if (track.chi2() > 0 && track.nDoF() > 0) {
      double p = 1. / std::log10(10. + 10. * track.chi2() / track.nDoF());
      if (p > 0) {
        score += p;
      } else {
        score -= 50;
      }
    }

    iTrack++;

    // Add the score to the vector
    trackScore.push_back(score);
    ACTS_VERBOSE("Track: " << iTrack << " score: " << score);

  }  // end of loop over tracks

  return trackScore;
}

template <TrackContainerFrontend track_container_t>
std::vector<double> Acts::ScoreBasedAmbiguityResolution::ambiguityScore(
    const track_container_t& tracks,
    const std::vector<std::vector<TrackFeatures>>& trackFeaturesVectors,
    const OptionalCuts<typename track_container_t::ConstTrackProxy>&
        optionalCuts) const {
  std::vector<double> trackScore;
  trackScore.reserve(tracks.size());

  ACTS_VERBOSE("Using Ambiguity Scoring function");

  int iTrack = 0;

  ACTS_VERBOSE("Number of detectors: " << m_cfg.detectorConfigs.size());

  ACTS_INFO("Starting to score tracks");

  // Loop over all the tracks in the container
  for (const auto& track : tracks) {
    // get the trackFeatures map for the track
    const auto& trackFeaturesVector = trackFeaturesVectors[iTrack];
    double score = 1;
    auto pT = Acts::VectorHelpers::perp(track.momentum());
    auto eta = Acts::VectorHelpers::eta(track.momentum());
    auto phi = Acts::VectorHelpers::phi(track.momentum());
    // cuts on pT
    if (pT < m_cfg.pTMin || pT > m_cfg.pTMax) {
      score = 0;
      iTrack++;
      trackScore.push_back(score);
      ACTS_DEBUG("Track: " << iTrack
                           << " has score = 0, due to pT cuts --- pT = " << pT);
      continue;
    }

    // cuts on phi
    if (phi > m_cfg.phiMax || phi < m_cfg.phiMin) {
      score = 0;
      iTrack++;
      trackScore.push_back(score);
      ACTS_DEBUG("Track: " << iTrack
                           << " has score = 0, due to phi cuts --- phi =  "
                           << phi);
      continue;
    }

    // cuts on eta
    if (eta > m_cfg.etaMax || eta < m_cfg.etaMin) {
      score = 0;
      iTrack++;
      trackScore.push_back(score);
      ACTS_DEBUG("Track: " << iTrack
                           << " has score = 0, due to eta cuts --- eta =  "
                           << eta);
      continue;
    }

    // cuts on optional cuts
    for (const auto& cutFunction : optionalCuts.cuts) {
      if (cutFunction(track)) {
        score = 0;
        ACTS_DEBUG("Track: " << iTrack
                             << " has score = 0, due to optional cuts.");
        break;
      }
    }

    if (score == 0) {
      iTrack++;
      trackScore.push_back(score);
      ACTS_DEBUG("Track: " << iTrack << " score : " << score);
      continue;
    }
    // Reject tracks which didn't pass the detector cuts.
    for (std::size_t detectorId = 0; detectorId < m_cfg.detectorConfigs.size();
         detectorId++) {
      const auto& detector = m_cfg.detectorConfigs.at(detectorId);

      const auto& trackFeatures = trackFeaturesVector[detectorId];

      ACTS_DEBUG("---> Found summary information");
      ACTS_DEBUG("---> Detector ID: " << detectorId);
      ACTS_DEBUG("---> Number of hits: " << trackFeatures.nHits);
      ACTS_DEBUG("---> Number of holes: " << trackFeatures.nHoles);
      ACTS_DEBUG("---> Number of outliers: " << trackFeatures.nOutliers);

      if ((trackFeatures.nHits < detector.minHits) ||
          (trackFeatures.nHoles > detector.maxHoles) ||
          (trackFeatures.nOutliers > detector.maxOutliers)) {
        score = 0;
        ACTS_DEBUG("Track: " << iTrack
                             << " has score = 0, due to detector cuts");
        break;
      }
    }

    if (score == 0) {
      iTrack++;
      trackScore.push_back(score);
      ACTS_DEBUG("Track: " << iTrack << " score : " << score);
      continue;
    }

    // start with larger score for tracks with higher pT.
    score = std::log10(pT / UnitConstants::MeV) - 1.;
    // pT in GeV, hence 100 MeV is minimum and gets score = 1
    ACTS_DEBUG("Modifier for pT = " << pT << " GeV is : " << score
                                    << "  New score now: " << score);

    for (std::size_t detectorId = 0; detectorId < m_cfg.detectorConfigs.size();
         detectorId++) {
      const auto& detector = m_cfg.detectorConfigs.at(detectorId);

      const auto& trackFeatures = trackFeaturesVector[detectorId];

      // choosing a scaling factor based on the number of hits in a track per
      // detector.
      std::size_t nHits = trackFeatures.nHits;
      if (detector.factorHits.size() < nHits) {
        ACTS_WARNING("Detector " << detectorId
                                 << " has not enough factorhits in the "
                                    "detector.factorHits vector");
        continue;
      }
      if (nHits > detector.maxHits) {
        score = score * (detector.maxHits - nHits + 1);  // hits are good !
        nHits = detector.maxHits;
      }
      score = score * detector.factorHits[nHits];
      ACTS_DEBUG("Modifier for " << nHits
                                 << " hits: " << detector.factorHits[nHits]
                                 << "  New score now: " << score);

      // choosing a scaling factor based on the number of holes in a track per
      // detector.
      std::size_t iHoles = trackFeatures.nHoles;
      if (detector.factorHoles.size() < iHoles) {
        ACTS_WARNING("Detector " << detectorId
                                 << " has not enough factorholes in the "
                                    "detector.factorHoles vector");
        continue;
      }
      if (iHoles > detector.maxHoles) {
        score /= (iHoles - detector.maxHoles + 1);  // holes are bad !
        iHoles = detector.maxHoles;
      }
      score = score * detector.factorHoles[iHoles];
      ACTS_DEBUG("Modifier for " << iHoles
                                 << " holes: " << detector.factorHoles[iHoles]
                                 << "  New score now: " << score);
    }

    for (const auto& scoreFunction : optionalCuts.scores) {
      scoreFunction(track, score);
    }

    if (track.chi2() > 0 && track.nDoF() > 0) {
      double chi2 = track.chi2();
      int indf = track.nDoF();
      double fac = 1. / std::log10(10. + 10. * chi2 / indf);
      score = score * fac;
      ACTS_DEBUG("Modifier for chi2 = " << chi2 << " and NDF = " << indf
                                        << " is : " << fac
                                        << "  New score now: " << score);
    }
    iTrack++;

    // Add the score to the vector
    trackScore.push_back(score);
    ACTS_VERBOSE("Track: " << iTrack << " score: " << score);

  }  // end of loop over tracks

  return trackScore;
}

template <TrackContainerFrontend track_container_t>
std::vector<int> Acts::ScoreBasedAmbiguityResolution::solveAmbiguity(
    const track_container_t& tracks,
    const std::vector<std::vector<MeasurementInfo>>& measurementsPerTrack,
    const std::vector<std::vector<TrackFeatures>>& trackFeaturesVectors,
    const OptionalCuts<typename track_container_t::ConstTrackProxy>&
        optionalCuts) const {
  ACTS_INFO("Number of tracks before Ambiguty Resolution: " << tracks.size());
  // vector of trackFeaturesVectors. where each trackFeaturesVector contains the
  // number of hits/hole/outliers for each detector in a track.

  std::vector<double> trackScore;
  trackScore.reserve(tracks.size());
  if (m_cfg.useAmbiguityFunction) {
    trackScore = ambiguityScore(tracks, trackFeaturesVectors, optionalCuts);
  } else {
    trackScore = simpleScore(tracks, trackFeaturesVectors, optionalCuts);
  }

  std::vector<bool> cleanTracks = getCleanedOutTracks(
      trackScore, trackFeaturesVectors, measurementsPerTrack);

  std::vector<int> goodTracks;
  int cleanTrackIndex = 0;
  std::size_t iTrack = 0;
  for (const auto& track : tracks) {
    if (cleanTracks[iTrack]) {
      cleanTrackIndex++;
      if (trackScore[iTrack] >= m_cfg.minScore) {
        goodTracks.push_back(track.index());
      }
    }
    iTrack++;
  }
  ACTS_VERBOSE("Number of clean tracks: " << cleanTrackIndex);
  ACTS_VERBOSE("Min score: " << m_cfg.minScore);
  ACTS_INFO("Number of Good tracks: " << goodTracks.size());
  return goodTracks;
}

}  // namespace Acts
