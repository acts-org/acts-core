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

template <TrackContainerFrontend track_container_t>
std::vector<std::vector<ScoreBasedAmbiguityResolution::TrackFeatures>>
ScoreBasedAmbiguityResolution::computeInitialState(
    const track_container_t& tracks) const {
  ACTS_VERBOSE("Starting to compute initial state");
  std::vector<std::vector<TrackFeatures>> trackFeaturesVectors;
  trackFeaturesVectors.reserve(tracks.size());

  for (const auto& track : tracks) {
    int numberOfDetectors = m_cfg.detectorConfigs.size();

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
        ACTS_VERBOSE("Track state type is HoleFlag");
        trackFeaturesVector[detectorId].nHoles++;
      } else if (ts.typeFlags().test(Acts::TrackStateFlag::OutlierFlag)) {
        ACTS_VERBOSE("Track state type is OutlierFlag");
        trackFeaturesVector[detectorId].nOutliers++;

      } else if (ts.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
        ACTS_VERBOSE("Track state type is MeasurementFlag");

        if (ts.typeFlags().test(Acts::TrackStateFlag::SharedHitFlag)) {
          trackFeaturesVector[detectorId].nSharedHits++;
        }
        trackFeaturesVector[detectorId].nHits++;
      }
    }
    trackFeaturesVectors.push_back(std::move(trackFeaturesVector));
  }

  return trackFeaturesVectors;
}

template <TrackContainerFrontend track_container_t>
std::vector<double> Acts::ScoreBasedAmbiguityResolution::simpleScore(
    const track_container_t& tracks,
    const std::vector<std::vector<TrackFeatures>>& trackFeaturesVectors,
    const Optionals<typename track_container_t::ConstTrackProxy>& optionals)
    const {
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
    auto eta = Acts::VectorHelpers::eta(track.momentum());

    // cuts on optional cuts
    for (const auto& cutFunction : optionals.cuts) {
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

      ACTS_VERBOSE("---> Found summary information");
      ACTS_VERBOSE("---> Detector ID: " << detectorId);
      ACTS_VERBOSE("---> Number of hits: " << trackFeatures.nHits);
      ACTS_VERBOSE("---> Number of holes: " << trackFeatures.nHoles);
      ACTS_VERBOSE("---> Number of outliers: " << trackFeatures.nOutliers);

      // eta based cuts
      if (etaBasedCuts(detector, trackFeatures, eta)) {
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

    // All cuts passed, now start scoring the track

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
    for (const auto& weightFunction : optionals.weights) {
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
    const Optionals<typename track_container_t::ConstTrackProxy>& optionals)
    const {
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

    // cuts on optional cuts
    for (const auto& cutFunction : optionals.cuts) {
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

      ACTS_VERBOSE("---> Found summary information");
      ACTS_VERBOSE("---> Detector ID: " << detectorId);
      ACTS_VERBOSE("---> Number of hits: " << trackFeatures.nHits);
      ACTS_VERBOSE("---> Number of holes: " << trackFeatures.nHoles);
      ACTS_VERBOSE("---> Number of outliers: " << trackFeatures.nOutliers);

      // eta based cuts
      if (etaBasedCuts(detector, trackFeatures, eta)) {
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

    // All cuts passed, now start scoring the track

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
      if (nHits > detector.maxHits) {
        score = score * (nHits - detector.maxHits + 1);  // hits are good !
        nHits = detector.maxHits;
      }
      score = score * detector.factorHits[nHits];
      ACTS_DEBUG("Modifier for " << nHits
                                 << " hits: " << detector.factorHits[nHits]
                                 << "  New score now: " << score);

      // choosing a scaling factor based on the number of holes in a track per
      // detector.
      std::size_t iHoles = trackFeatures.nHoles;
      if (iHoles > detector.maxHoles) {
        score /= (iHoles - detector.maxHoles + 1);  // holes are bad !
        iHoles = detector.maxHoles;
      }
      score = score * detector.factorHoles[iHoles];
      ACTS_DEBUG("Modifier for " << iHoles
                                 << " holes: " << detector.factorHoles[iHoles]
                                 << "  New score now: " << score);
    }

    for (const auto& scoreFunction : optionals.scores) {
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

template <TrackContainerFrontend track_container_t, typename source_link_hash_t,
          typename source_link_equality_t>
std::vector<int> Acts::ScoreBasedAmbiguityResolution::solveAmbiguity(
    const track_container_t& tracks, source_link_hash_t sourceLinkHash,
    source_link_equality_t sourceLinkEquality,
    const Optionals<typename track_container_t::ConstTrackProxy>& optionals)
    const {
  ACTS_INFO("Number of tracks before Ambiguty Resolution: " << tracks.size());
  // vector of trackFeaturesVectors. where each trackFeaturesVector contains the
  // number of hits/hole/outliers for each detector in a track.

  const std::vector<std::vector<TrackFeatures>> trackFeaturesVectors =
      computeInitialState<track_container_t>(tracks);

  std::vector<double> trackScore;
  trackScore.reserve(tracks.size());
  if (m_cfg.useAmbiguityScoring) {
    trackScore = ambiguityScore(tracks, trackFeaturesVectors, optionals);
  } else {
    trackScore = simpleScore(tracks, trackFeaturesVectors, optionals);
  }

  auto MeasurementIndexMap =
      std::unordered_map<SourceLink, std::size_t, source_link_hash_t,
                         source_link_equality_t>(0, sourceLinkHash,
                                                 sourceLinkEquality);

  std::vector<std::vector<std::size_t>> measurementsPerTrackVector;
  std::map<std::size_t, std::size_t> nTracksPerMeasurement;

  // Stores tracks measurement into a vector or vectors
  // (measurementsPerTrackVector) and counts the number of tracks per
  // measurement.(nTracksPerMeasurement)

  for (const auto& track : tracks) {
    std::vector<std::size_t> measurementsPerTrack;
    for (const auto& ts : track.trackStatesReversed()) {
      if (!ts.typeFlags().test(Acts::TrackStateFlag::OutlierFlag) &&
          !ts.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
        continue;
      }
      Acts::SourceLink sourceLink = ts.getUncalibratedSourceLink();
      // assign a new measurement index if the source link was not seen yet
      auto emplace = MeasurementIndexMap.try_emplace(
          sourceLink, MeasurementIndexMap.size());
      std::size_t iMeasurement = emplace.first->second;
      measurementsPerTrack.push_back(iMeasurement);
      if (nTracksPerMeasurement.find(iMeasurement) ==
          nTracksPerMeasurement.end()) {
        nTracksPerMeasurement[iMeasurement] = 0;
      }
      nTracksPerMeasurement[iMeasurement]++;
    }
    measurementsPerTrackVector.push_back(std::move(measurementsPerTrack));
  }

  std::vector<int> goodTracks;
  int cleanTrackIndex = 0;

  auto optionalHitSelections = optionals.hitSelections;

  // Loop over all the tracks in the container
  // For each track, check if the track has too many shared hits to be accepted.
  // If the track is good, add it to the goodTracks
  for (std::size_t iTrack = 0; const auto& track : tracks) {
    // Check if the track has too many shared hits to be accepted.
    if (getCleanedOutTracks(track, trackScore[iTrack],
                            measurementsPerTrackVector[iTrack],
                            nTracksPerMeasurement, optionalHitSelections)) {
      cleanTrackIndex++;
      if (trackScore[iTrack] > m_cfg.minScore) {
        goodTracks.push_back(track.index());
      }
    }
    iTrack++;
  }
  ACTS_INFO("Number of clean tracks: " << cleanTrackIndex);
  ACTS_VERBOSE("Min score: " << m_cfg.minScore);
  ACTS_INFO("Number of Good tracks: " << goodTracks.size());
  return goodTracks;
}

template <TrackProxyConcept track_proxy_t>
bool Acts::ScoreBasedAmbiguityResolution::getCleanedOutTracks(
    const track_proxy_t& track, const double& trackScore,
    const std::vector<std::size_t>& measurementsPerTrack,
    const std::map<std::size_t, std::size_t>& nTracksPerMeasurement,
    const std::vector<
        std::function<void(const track_proxy_t&,
                           const typename track_proxy_t::ConstTrackStateProxy&,
                           TrackStateTypes&)>>& optionalHitSelections) const {
  // For tracks with shared hits, we need to check and remove bad hits

  std::vector<TrackStateTypes> trackStateTypes;
  // Loop over all measurements of the track and for each hit a
  // trackStateTypes is assigned.
  for (std::size_t index = 0; const auto& ts : track.trackStatesReversed()) {
    if (ts.typeFlags().test(Acts::TrackStateFlag::OutlierFlag) ||
        ts.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
      std::size_t iMeasurement = measurementsPerTrack[index];
      auto it = nTracksPerMeasurement.find(iMeasurement);
      if (it == nTracksPerMeasurement.end()) {
        trackStateTypes.push_back(TrackStateTypes::OtherTrackStateType);
        index++;
        continue;
      }

      std::size_t nTracksShared = it->second;
      auto isoutliner = ts.typeFlags().test(Acts::TrackStateFlag::OutlierFlag);

      if (isoutliner) {
        ACTS_VERBOSE("Measurement is outlier on a fitter track, copy it over");
        trackStateTypes.push_back(TrackStateTypes::Outlier);
        continue;
      }
      if (nTracksShared == 1) {
        ACTS_VERBOSE("Measurement is not shared, copy it over");

        trackStateTypes.push_back(TrackStateTypes::UnsharedHit);
        continue;
      } else if (nTracksShared > 1) {
        ACTS_VERBOSE("Measurement is shared, copy it over");
        trackStateTypes.push_back(TrackStateTypes::SharedHit);
        continue;
      }
    }
  }
  std::vector<std::size_t> newMeasurementsPerTrack;
  std::size_t measurement = 0;
  std::size_t nshared = 0;

  // Loop over all measurements of the track and process them according to the
  // trackStateTypes and other conditions.
  // Good measurements are copied to the newMeasurementsPerTrack vector.
  for (std::size_t index = 0; auto ts : track.trackStatesReversed()) {
    if (ts.typeFlags().test(Acts::TrackStateFlag::OutlierFlag) ||
        ts.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
      if (!ts.hasReferenceSurface()) {
        ACTS_DEBUG("Track state has no reference surface");
        continue;
      }

      std::size_t ivolume = ts.referenceSurface().geometryId().volume();
      auto volume_it = m_cfg.volumeMap.find(ivolume);
      if (volume_it == m_cfg.volumeMap.end()) {
        ACTS_ERROR("Volume " << ivolume << " not found in the volume map");
        continue;
      }

      std::size_t detectorID = volume_it->second;

      const auto& detector = m_cfg.detectorConfigs.at(detectorID);

      measurement = measurementsPerTrack[index];

      auto it = nTracksPerMeasurement.find(measurement);
      if (it == nTracksPerMeasurement.end()) {
        index++;
        continue;
      }
      auto nTracksShared = it->second;

      // Loop over all optionalHitSelections and apply them to trackStateType of
      // the TrackState.

      for (const auto& hitSelection : optionalHitSelections) {
        hitSelection(track, ts, trackStateTypes[index]);
      }

      if (trackStateTypes[index] == TrackStateTypes::RejectedHit) {
        ACTS_DEBUG("Dropping rejected hit");
      } else if (trackStateTypes[index] != TrackStateTypes::SharedHit) {
        ACTS_DEBUG("Good TSOS, copy hit");
        newMeasurementsPerTrack.push_back(measurement);

        // a counter called nshared is used to keep track of the number of
        // shared hits accepted.
      } else if (nshared >= m_cfg.maxShared) {
        ACTS_DEBUG("Too many shared hit, drop it");
      }
      // If the track is shared, the hit is only accepted if the track has
      // score higher than the minimum score for shared tracks.
      else {
        ACTS_DEBUG("Try to recover shared hit ");
        if (nTracksShared <= m_cfg.maxSharedTracksPerMeasurement &&
            trackScore > m_cfg.minScoreSharedTracks &&
            !detector.sharedHitsFlag) {
          ACTS_DEBUG("Accepted hit shared with " << nTracksShared << " tracks");
          newMeasurementsPerTrack.push_back(measurement);
          nshared++;
        } else {
          ACTS_DEBUG("Rejected hit shared with " << nTracksShared << " tracks");
        }
      }
      index++;
    }
  }
  // Check if the track has enough hits to be accepted.
  if (newMeasurementsPerTrack.size() < m_cfg.minUnshared) {
    return false;
  } else {
    return true;
  }
}

}  // namespace Acts
