// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
