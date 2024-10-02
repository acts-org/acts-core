// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"

#include <map>
#include <string>
#include <vector>

namespace ActsExamples {

/// Generic implementation of the machine learning ambiguity resolution
/// Contains method for data preparations
class AmbiguityResolutionML : public IAlgorithm {
 public:
  /// Construct the ambiguity resolution algorithm.
  ///
  /// @param name name of the algorithm
  /// @param lvl is the logging level
  AmbiguityResolutionML(std::string name, Acts::Logging::Level lvl);

 protected:
  /// Associated measurements ID to Tracks ID
  ///
  /// @param tracks is the input track container
  /// @param nMeasurementsMin minimum number of measurement per track
  /// @return an ordered list containing pairs of track ID and associated measurement ID
  std::multimap<int, std::pair<std::size_t, std::vector<std::size_t>>>
  mapTrackHits(const ConstTrackContainer& tracks, int nMeasurementsMin) const;

  /// Prepare the output track container to be written
  ///
  /// @param tracks is the input track container
  /// @param goodTracks is list of the IDs of all the tracks we want to keep
  ConstTrackContainer prepareOutputTrack(
      const ConstTrackContainer& tracks,
      std::vector<std::size_t>& goodTracks) const;
};

}  // namespace ActsExamples
