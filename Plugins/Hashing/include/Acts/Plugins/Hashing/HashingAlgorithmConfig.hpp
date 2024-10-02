// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

namespace Acts {
struct HashingAlgorithmConfig {
  /// Size of the buckets = number of spacepoints in the bucket
  unsigned int bucketSize = 10;
  /// Number of zBins
  unsigned int zBins = 0;
  /// Number of phiBins
  unsigned int phiBins = 50;

  /// Layer selection
  double layerRMin = 25;
  double layerRMax = 40;
  double layerZMin = -550;
  double layerZMax = 550;
};

}  // namespace Acts
