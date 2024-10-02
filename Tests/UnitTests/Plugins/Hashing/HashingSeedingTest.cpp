// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Plugins/Hashing/HashingAlgorithm.hpp"
#include "Acts/Plugins/Hashing/HashingAlgorithmConfig.hpp"
#include "Acts/Plugins/Hashing/HashingAnnoy.hpp"
#include "Acts/Plugins/Hashing/HashingTraining.hpp"
#include "Acts/Plugins/Hashing/HashingTrainingConfig.hpp"
#include "Acts/Seeding/BinnedGroup.hpp"
#include "Acts/Seeding/detail/UtilityFunctions.hpp"

#include <cmath>
#include <cstdlib>
#include <memory>
#include <vector>

#include <annoy/annoylib.h>
#include <annoy/kissrandom.h>

#include "SpacePoint.hpp"

using namespace Acts::UnitLiterals;

// Function to create and initialize the test vector
std::vector<std::unique_ptr<const SpacePoint>> createTestVector() {
  std::optional<float> t, varianceT;
  std::vector<std::unique_ptr<const SpacePoint>> testVector;
  testVector.reserve(6);  // Reserve space for efficiency

  testVector.push_back(std::make_unique<SpacePoint>(
      27.2535, -18.0088, -146.526, 29.0, 1, 0.00520833, 0.5, t, varianceT));
  testVector.push_back(std::make_unique<SpacePoint>(
      42.9126, -27.3057, -171.477, 50.0, 1, 0.0133333, 0.8, t, varianceT));
  testVector.push_back(std::make_unique<SpacePoint>(
      42.7087, -27.4589, -171.557, 50.0, 1, 0.0133333, 0.4, t, varianceT));
  testVector.push_back(std::make_unique<SpacePoint>(
      74.3652, -45.8552, -221.277, 80.0, 1, 0.0133333, 0.4, t, varianceT));
  testVector.push_back(std::make_unique<SpacePoint>(
      104.12, -63.4203, -268.468, 110.0, 1, 0.0133333, 0.4, t, varianceT));
  testVector.push_back(std::make_unique<SpacePoint>(
      104.412, -63.1851, -268.468, 110.0, 1, 0.0133333, 0.4, t, varianceT));
  return testVector;
}
namespace Acts::Test {
BOOST_AUTO_TEST_CASE(HashingBucketCreationTest) {
  using SpacePointPtrVector = std::vector<const SpacePoint*>;

  // Initialize testVector using the createTestVector function
  auto testVector = createTestVector();

  // Extract raw pointers from unique_ptrs for the test
  SpacePointPtrVector spVec;
  spVec.reserve(testVector.size());
  for (const auto& sp : testVector) {
    spVec.push_back(sp.get());
  }

  /// Random seed for Annoy
  unsigned int annoySeed = 123456789;

  /// Number of features to use
  std::int32_t nf = 1;

  /// Size of the buckets = number of spacepoints in the bucket
  unsigned int bucketSize = 100;
  /// Number of zBins
  unsigned int zBins = 10000;
  /// Number of phiBins
  unsigned int phiBins = 0;

  /// Layer selection
  double layerRMin = 25;
  double layerRMax = 40;
  double layerZMin = -550;
  double layerZMax = 550;

  Acts::HashingAlgorithmConfig hashingConfig;
  hashingConfig.bucketSize = bucketSize;
  hashingConfig.zBins = zBins;
  hashingConfig.phiBins = phiBins;
  hashingConfig.layerRMin = layerRMin;
  hashingConfig.layerRMax = layerRMax;
  hashingConfig.layerZMin = layerZMin;
  hashingConfig.layerZMax = layerZMax;

  Acts::HashingTrainingConfig hashingTrainingConfig;
  hashingTrainingConfig.annoySeed = annoySeed;
  hashingTrainingConfig.f = nf;

  Acts::HashingAlgorithm<const SpacePoint*, SpacePointPtrVector> hashing =
      Acts::HashingAlgorithm<const SpacePoint*, SpacePointPtrVector>(
          hashingConfig);
  Acts::HashingTrainingAlgorithm<SpacePointPtrVector> hashingTraining =
      Acts::HashingTrainingAlgorithm<SpacePointPtrVector>(
          hashingTrainingConfig);

  // Hashing Training
  Acts::AnnoyModel annoyModel = hashingTraining.execute(spVec);

  // Hashing
  std::vector<SpacePointPtrVector> bucketsPtrs;
  bucketsPtrs.clear();
  hashing.execute(spVec, &annoyModel, bucketsPtrs);

  // Check the number of buckets created
  BOOST_CHECK_GT(bucketsPtrs.size(), 0);
}

BOOST_AUTO_TEST_CASE(HashingBucketContentTest) {
  using SpacePointPtrVector = std::vector<const SpacePoint*>;

  // Initialize testVector using the createTestVector function
  auto testVector = createTestVector();

  // Extract raw pointers from unique_ptrs for the test
  SpacePointPtrVector spVec;
  spVec.reserve(testVector.size());
  for (const auto& sp : testVector) {
    spVec.push_back(sp.get());
  }

  /// Random seed for Annoy
  unsigned int annoySeed = 123456789;

  /// Number of features to use
  std::int32_t nf = 1;

  /// Size of the buckets = number of spacepoints in the bucket
  unsigned int bucketSize = 100;
  /// Number of zBins
  unsigned int zBins = 10000;
  /// Number of phiBins
  unsigned int phiBins = 0;

  /// Layer selection
  double layerRMin = 25;
  double layerRMax = 40;
  double layerZMin = -550;
  double layerZMax = 550;

  Acts::HashingAlgorithmConfig hashingConfig;
  hashingConfig.bucketSize = bucketSize;
  hashingConfig.zBins = zBins;
  hashingConfig.phiBins = phiBins;
  hashingConfig.layerRMin = layerRMin;
  hashingConfig.layerRMax = layerRMax;
  hashingConfig.layerZMin = layerZMin;
  hashingConfig.layerZMax = layerZMax;

  Acts::HashingTrainingConfig hashingTrainingConfig;
  hashingTrainingConfig.annoySeed = annoySeed;
  hashingTrainingConfig.f = nf;

  Acts::HashingAlgorithm<const SpacePoint*, SpacePointPtrVector> hashing =
      Acts::HashingAlgorithm<const SpacePoint*, SpacePointPtrVector>(
          hashingConfig);
  Acts::HashingTrainingAlgorithm<SpacePointPtrVector> hashingTraining =
      Acts::HashingTrainingAlgorithm<SpacePointPtrVector>(
          hashingTrainingConfig);

  // Hashing Training
  Acts::AnnoyModel annoyModel = hashingTraining.execute(spVec);

  // Hashing
  std::vector<SpacePointPtrVector> bucketsPtrs;
  bucketsPtrs.clear();
  hashing.execute(spVec, &annoyModel, bucketsPtrs);

  // Validate bucket content
  for (const auto& bucket : bucketsPtrs) {
    BOOST_CHECK_LE(bucket.size(), bucketSize);
    for (const auto* sp : bucket) {
      BOOST_CHECK(sp != nullptr);
    }
  }
}
}  // namespace Acts::Test
