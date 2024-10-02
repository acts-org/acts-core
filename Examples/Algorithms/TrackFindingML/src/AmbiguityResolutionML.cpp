// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TrackFindingML/AmbiguityResolutionML.hpp"

#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"

ActsExamples::AmbiguityResolutionML::AmbiguityResolutionML(
    std::string name, Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm(name, lvl) {}

std::multimap<int, std::pair<std::size_t, std::vector<std::size_t>>>
ActsExamples::AmbiguityResolutionML::mapTrackHits(
    const ActsExamples::ConstTrackContainer& tracks,
    int nMeasurementsMin) const {
  std::multimap<int, std::pair<std::size_t, std::vector<std::size_t>>> trackMap;
  // Loop over all the trajectories in the events
  for (const auto& track : tracks) {
    std::vector<std::size_t> hits;
    int nbMeasurements = 0;
    // Store the hits id for the trajectory and compute the number of
    // measurement
    tracks.trackStateContainer().visitBackwards(
        track.tipIndex(), [&](const auto& state) {
          if (state.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
            std::size_t indexHit =
                state.getUncalibratedSourceLink()
                    .template get<ActsExamples::IndexSourceLink>()
                    .index();
            hits.emplace_back(indexHit);
            ++nbMeasurements;
          }
        });
    if (nbMeasurements < nMeasurementsMin) {
      continue;
    }
    trackMap.emplace(nbMeasurements, std::make_pair(track.index(), hits));
  }
  return trackMap;
}

ActsExamples::ConstTrackContainer
ActsExamples::AmbiguityResolutionML::prepareOutputTrack(
    const ActsExamples::ConstTrackContainer& tracks,
    std::vector<std::size_t>& goodTracks) const {
  std::shared_ptr<Acts::ConstVectorMultiTrajectory> trackStateContainer =
      tracks.trackStateContainerHolder();
  auto trackContainer = std::make_shared<Acts::VectorTrackContainer>();
  trackContainer->reserve(goodTracks.size());
  // Temporary empty track state container: we don't change the original one,
  // but we need one for filtering
  auto tempTrackStateContainer =
      std::make_shared<Acts::VectorMultiTrajectory>();

  TrackContainer solvedTracks{trackContainer, tempTrackStateContainer};
  solvedTracks.ensureDynamicColumns(tracks);

  for (auto&& iTrack : goodTracks) {
    auto destProxy = solvedTracks.makeTrack();
    auto srcProxy = tracks.getTrack(iTrack);
    destProxy.copyFrom(srcProxy, false);
    destProxy.tipIndex() = srcProxy.tipIndex();
  }

  ConstTrackContainer outputTracks{
      std::make_shared<Acts::ConstVectorTrackContainer>(
          std::move(*trackContainer)),
      trackStateContainer};
  return outputTracks;
}
