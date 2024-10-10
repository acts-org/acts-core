// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/AmbiguityResolution/ScoreBasedAmbiguityResolution.hpp"

#include "Acts/EventData/SourceLink.hpp"

#include <stdexcept>

std::vector<bool> Acts::ScoreBasedAmbiguityResolution::getCleanedOutTracks(
    const std::vector<double>& trackScore,
    const std::vector<std::vector<TrackFeatures>>& trackFeaturesVectors,
    const std::vector<std::vector<MeasurementInfo>>& measurementsPerTrack)
    const {
  std::vector<bool> cleanTracks(measurementsPerTrack.size(), false);

  ACTS_VERBOSE("Cleaning tracks");

  if (trackScore.size() != measurementsPerTrack.size()) {
    throw std::invalid_argument(
        "Track score and measurementsPerTrack size mismatch");
  }

  std::size_t numberOfTracks = measurementsPerTrack.size();
  ACTS_DEBUG("Number of tracks: " << numberOfTracks);

  boost::container::flat_map<std::size_t,
                             boost::container::flat_set<std::size_t>>
      tracksPerMeasurement;

  // Removes bad tracks and counts computes the vector of tracks per
  // measurement.
  for (std::size_t iTrack = 0; iTrack < numberOfTracks; ++iTrack) {
    if (trackScore[iTrack] <= 0) {
      continue;
    }
    for (auto measurementObjects : measurementsPerTrack[iTrack]) {
      auto iMeasurement = measurementObjects.iMeasurement;
      tracksPerMeasurement[iMeasurement].insert(iTrack);
    }
  }

  enum TrackStateTypes {
    // A measurement not yet used in any other track
    UnsharedHit = 1,
    // A measurement shared with another track
    SharedHit = 2,
    // A hit that needs to be removed from the track
    RejectedHit = 3,
    // an outlier, to be copied in case
    Outlier = 4,
    // other trackstate types to be copied in case
    OtherTrackStateType = 5
  };

  std::vector<std::vector<std::size_t>> newMeasurements;
  // Loop over all tracks in the track container
  for (std::size_t iTrack = 0; iTrack < numberOfTracks; ++iTrack) {
    double track_score = trackScore[iTrack];
    ACTS_DEBUG("Track score: " << track_score);

    if (track_score <= 0) {
      ACTS_DEBUG("Track " << iTrack << " could not be accepted - low score");
      continue;
    }

    const auto& trackFeaturesVector = trackFeaturesVectors.at(iTrack);

    bool trkCouldBeAccepted = true;

    // For tracks with shared hits, we need to check and remove bad hits

    std::vector<int> trackStateTypes(measurementsPerTrack[iTrack].size(),
                                     OtherTrackStateType);
    int index = 0;

    // Loop over all measurements of the track and for each hit a
    // trackStateTypes is assigned.
    for (const auto& measurementObjects : measurementsPerTrack[iTrack]) {
      auto iMeasurement = measurementObjects.iMeasurement;
      auto isoutliner = measurementObjects.isOutlier;
      auto detectorId = measurementObjects.detectorId;

      auto detector = m_cfg.detectorConfigs.at(detectorId);
      if (isoutliner) {
        ACTS_VERBOSE("Measurement is outlier on a fitter track, copy it over");
        trackStateTypes[index] = Outlier;
        index++;
        continue;
      }
      if (tracksPerMeasurement[iMeasurement].size() == 1) {
        ACTS_VERBOSE("Measurement is not shared, copy it over");

        trackStateTypes[index] = UnsharedHit;

        index++;
        continue;
      }
      if (tracksPerMeasurement[iMeasurement].size() > 1) {
        ACTS_VERBOSE("Measurement is shared, copy it over");

        if (detector.sharedHitsFlag == true) {
          ACTS_VERBOSE("Measurement is shared, Reject it");
          trackStateTypes[index] = RejectedHit;
          index++;
          continue;
        }

        trackStateTypes[index] = SharedHit;

        index++;
        continue;
      }
    }

    std::vector<std::size_t> newMeasurementsPerTrack;
    std::size_t measurement = 0;
    std::size_t nshared = 0;

    // Loop over all measurements of the track and process them according to the
    // trackStateTypes and other conditions.
    // Good measurements are copied to the newMeasurementsPerTrack vector.
    for (std::size_t i = 0; i < trackStateTypes.size(); i++) {
      auto& measurementObjects = measurementsPerTrack[iTrack][i];
      measurement = measurementObjects.iMeasurement;

      if (trackStateTypes[i] == RejectedHit) {
        ACTS_DEBUG("Dropping rejected hit");
      } else if (trackStateTypes[i] != SharedHit) {
        ACTS_DEBUG("Good TSOS, copy hit");
        newMeasurementsPerTrack.push_back(measurement);

        // a counter called nshared is used to keep track of the number of
        // shared hits accepted.
      } else if (nshared >= m_cfg.maxShared) {
        ACTS_DEBUG("Too many shared hit, drop it");
      }
      // If the track is shared, the hit is only accepted if the track has score
      // higher than the minimum score for shared tracks.
      else {
        ACTS_DEBUG("Try to recover shared hit ");
        if (tracksPerMeasurement[measurement].size() <
                m_cfg.maxSharedTracksPerMeasurement &&
            track_score > m_cfg.minScoreSharedTracks) {
          ACTS_DEBUG("Accepted hit shared with "
                     << tracksPerMeasurement[measurement].size() << " tracks");
          newMeasurementsPerTrack.push_back(measurement);
          nshared++;
        } else {
          ACTS_DEBUG("Rejected hit shared with "
                     << tracksPerMeasurement[measurement].size() << " tracks");
        }
      }
    }

    // Check if the track has enough hits to be accepted.
    if (newMeasurementsPerTrack.size() < 3) {
      trkCouldBeAccepted = false;
      ACTS_DEBUG(std::endl
                 << "Track " << iTrack
                 << " could not be accepted - not enough hits");
      ACTS_DEBUG("Number of hits: " << measurementsPerTrack[iTrack].size());
      ACTS_DEBUG("Number of good hits: " << newMeasurementsPerTrack.size());
      continue;
    }

    // Check if the track has too many shared hits to be accepted.
    for (std::size_t detectorId = 0; detectorId < m_cfg.detectorConfigs.size();
         detectorId++) {
      auto detector = m_cfg.detectorConfigs.at(detectorId);
      if (trackFeaturesVector[detectorId].nSharedHits >
          detector.maxSharedHits) {
        trkCouldBeAccepted = false;
        break;
      }
    }

    if (trkCouldBeAccepted) {
      cleanTracks[iTrack] = true;
      newMeasurements.push_back(newMeasurementsPerTrack);
      ACTS_VERBOSE("Track " << iTrack << " is accepted");
      continue;
    }
  }

  return cleanTracks;
}
