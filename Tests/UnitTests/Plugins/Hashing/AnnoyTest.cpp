// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include <array>
#include <cstdint>
#include <vector>

#include <annoy/annoylib.h>
#include <annoy/kissrandom.h>

namespace {
// Test vector of 2D points generated from the make_blobs function of
// scikit-learn to correspond to 4 clusters with a standard deviation of 0.3
std::vector<std::array<double, 2>> testVector{
    {-2.83739915, 2.62792556},  {-2.02847331, -1.90722196},
    {4.42609249, -2.42439165},  {-2.54167208, -1.31586441},
    {-2.74072011, 1.88175176},  {-2.44805173, -1.72270269},
    {4.32373114, -3.04946856},  {-3.02908065, 3.05502207},
    {4.21551681, -2.72367792},  {6.94454243, -8.26702198},
    {4.57729285, -2.98832874},  {-2.05999536, -1.60664607},
    {7.29942963, -7.49254664},  {-1.76560555, -1.94316957},
    {-3.08697607, 2.38012823},  {-2.68133439, -1.96863594},
    {-3.04707961, 2.42381653},  {-1.6693666 - 1.98996212},
    {4.87565038, -2.42067792},  {6.57829525 - 8.14269767},
    {-1.89777458, -1.71029565}, {-2.82010574, 2.27963425},
    {-1.8938416, -1.76676642},  {-2.8088788, 2.14373147},
    {-2.7111892, 2.7343114},    {5.00997563, -3.03311654},
    {-3.00272791, 1.59086316},  {-2.69800242, 2.19671366},
    {5.35757875, -2.98359632},  {6.41134781, -7.79582109},
    {5.06123223, -2.84952632},  {6.33969189, -7.83811637},
    {5.11101701, -2.80965778},  {7.01442234, -7.47047664},
    {6.82239627, -7.97467806},  {6.82647513, -7.64299033},
    {-2.02651791, -1.81791892}, {-2.53859699, -2.20157508},
    {5.07240334, -2.48183097},  {-1.58869273, -2.30974576},
    {5.24011121, -2.78045434},  {4.89256735, -2.98154234},
    {-2.61589554, -1.38994103}, {-2.37898031, 2.02633106},
    {6.71148996, -7.87697906},  {-2.24310299, -2.01958434},
    {4.80875851, -3.00716459},  {-2.20240163, -1.45942015},
    {5.0317719, -3.33571147},   {4.68497184, -2.2794554},
    {6.57950453, -7.84613618},  {-2.39557904, -0.97990746},
    {4.89489222, -3.31597619},  {5.22670358, -2.79577779},
    {4.87625814, -2.70562793},  {5.37121464, -2.78439938},
    {6.48510206, -7.89652351},  {-2.78153003, 1.79940689},
    {6.80163025, -7.7267214},   {-2.42494396, -1.95543603},
    {7.01502605, -7.93922357},  {-2.00219795, -1.95198446},
    {-2.82690524, 1.83749478},  {-2.81153684, 2.30020325},
    {-1.46316156, -1.70854783}, {-2.36754202, -1.62836379},
    {-3.12179904, 1.86079695},  {-2.80228975, 2.16674687},
    {7.25447808, -7.87780152},  {6.34182023, -7.72244414},
    {6.85296593, -7.6565112},   {6.40782187, -7.95817435},
    {4.60981662, -2.6214774},   {6.82470403, -7.8453859},
    {-2.94909893, 2.4408267},   {6.48588252, -8.42529572},
    {6.55194867, -7.54354929},  {-2.64178285, 2.28031333},
    {-1.95664147, -2.44817923}, {-2.00957937, -2.01412199},
    {-2.24603999, 2.48964234},  {4.73274418, -2.89077558},
    {-2.47534453, 1.85935482},  {-2.35722712, -1.99652695},
    {5.15661108, -2.88549784},  {6.68114631, -7.73743642},
    {4.93268708, -2.97510717},  {6.54260932, -8.82618456},
    {-3.57448792, 2.06852256},  {6.63296723, -8.32752766},
    {-3.58610661, 2.2761471},   {-2.73077783, 1.8138345},
    {-2.14150912, 1.94984708},  {-2.27235876, -1.67574786},
    {6.92208545, -8.46326386},  {4.58953972, -3.22764749},
    {-3.36912131, 2.58470911},  {5.28526348, -2.55723196},
    {6.55276593, -7.81387909},  {-1.79854507, -2.10170986}};
}  // namespace

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(AnnoySetSeedTest) {
  using AnnoyMetric = Annoy::Euclidean;
  using AnnoyModel =
      Annoy::AnnoyIndex<unsigned int, double, AnnoyMetric, Annoy::Kiss32Random,
                        Annoy::AnnoyIndexSingleThreadedBuildPolicy>;

  const unsigned int annoySeed = 123456789;
  const std::int32_t f = 2;

  AnnoyModel annoyModel = AnnoyModel(f);

  annoyModel.set_seed(annoySeed);

  BOOST_CHECK_EQUAL(annoyModel.get_seed(), annoySeed);
}

BOOST_AUTO_TEST_CASE(AnnoyAddAndBuildTest) {
  using AnnoyMetric = Annoy::Euclidean;
  using AnnoyModel =
      Annoy::AnnoyIndex<unsigned int, double, AnnoyMetric, Annoy::Kiss32Random,
                        Annoy::AnnoyIndexSingleThreadedBuildPolicy>;

  const unsigned int annoySeed = 123456789;
  const std::int32_t f = 2;

  AnnoyModel annoyModel = AnnoyModel(f);

  annoyModel.set_seed(annoySeed);

  unsigned int pointIndex = 0;
  // Add spacePoints parameters to Annoy
  for (const auto& arrayvec : testVector) {
    annoyModel.add_item(pointIndex, arrayvec.data());
    pointIndex++;
  }

  unsigned int nTrees = 2 * f;

  annoyModel.build(nTrees);

  /// Get the bucketSize closest spacePoints
  unsigned int bucketSize = 5;
  std::vector<unsigned int> bucketIds;
  annoyModel.get_nns_by_item(0, bucketSize, -1, &bucketIds, nullptr);

  BOOST_CHECK_EQUAL(bucketIds.size(), bucketSize);
}

BOOST_AUTO_TEST_CASE(AnnoyNeighborTest) {
  using AnnoyMetric = Annoy::Euclidean;
  using AnnoyModel =
      Annoy::AnnoyIndex<unsigned int, double, AnnoyMetric, Annoy::Kiss32Random,
                        Annoy::AnnoyIndexSingleThreadedBuildPolicy>;

  const unsigned int annoySeed = 123456789;
  const std::int32_t f = 2;

  AnnoyModel annoyModel = AnnoyModel(f);

  annoyModel.set_seed(annoySeed);

  unsigned int pointIndex = 0;
  // Add spacePoints parameters to Annoy
  for (const auto& arrayvec : testVector) {
    annoyModel.add_item(pointIndex, arrayvec.data());
    pointIndex++;
  }

  unsigned int nTrees = 2 * f;

  annoyModel.build(nTrees);

  /// Validate neighbors for the first point
  unsigned int bucketSize = 5;
  std::vector<unsigned int> bucketIds;
  std::vector<double> distances;
  annoyModel.get_nns_by_item(0, bucketSize, -1, &bucketIds, &distances);

  BOOST_CHECK_EQUAL(bucketIds.size(), bucketSize);
  BOOST_CHECK_EQUAL(distances.size(), bucketSize);

  // Check that the first item is the closest to itself
  BOOST_CHECK_EQUAL(bucketIds[0], 0);

  // Check the distances are sorted in ascending order
  for (std::size_t i = 1; i < distances.size(); ++i) {
    BOOST_CHECK(distances[i] >= distances[i - 1]);
  }
}

BOOST_AUTO_TEST_CASE(AnnoyDistanceTest) {
  using AnnoyMetric = Annoy::Euclidean;
  using AnnoyModel =
      Annoy::AnnoyIndex<unsigned int, double, AnnoyMetric, Annoy::Kiss32Random,
                        Annoy::AnnoyIndexSingleThreadedBuildPolicy>;

  const unsigned int annoySeed = 123456789;
  const std::int32_t f = 2;

  AnnoyModel annoyModel = AnnoyModel(f);

  annoyModel.set_seed(annoySeed);

  unsigned int pointIndex = 0;
  // Add spacePoints parameters to Annoy
  for (const auto& arrayvec : testVector) {
    annoyModel.add_item(pointIndex, arrayvec.data());
    pointIndex++;
  }

  unsigned int nTrees = 2 * f;

  annoyModel.build(nTrees);

  /// Validate the distance computation
  double distance = annoyModel.get_distance(0, 1);
  double expected_distance =
      std::sqrt(std::pow(testVector[0][0] - testVector[1][0], 2) +
                std::pow(testVector[0][1] - testVector[1][1], 2));

  BOOST_CHECK_CLOSE(distance, expected_distance, 1e-5);
}

}  // namespace Acts::Test
