// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Seeding/Seed.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"

#include <map>
#include <vector>

namespace ActsExamples {
/// Container of sim seed
using SimSeedContainer = std::vector<std::vector<Acts::Seed<SimSpacePoint>>>;

/// Struct for the grouped seed index
// @todo This might not be necessary if storing the seed as a ProtoTrack
struct GroupedSeedIndex {
  /// The region index
  Index regionIdx = 0;

  /// The seed index
  Index seedIdx = 0;
};

/// The track parameters to seed map
using TrackParametersSeedMap = std::map<Index, GroupedSeedIndex>;

}  // namespace ActsExamples
