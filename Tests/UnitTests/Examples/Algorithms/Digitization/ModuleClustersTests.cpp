// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/BinningData.hpp"
#include "ActsExamples/Digitization/ModuleClusters.hpp"
#include "ActsFatras/Digitization/Segmentizer.hpp"

using namespace Acts;
using namespace ActsFatras;
using namespace ActsExamples;

namespace {

DigitizedParameters makeDigitizationParameters(const Vector2 &position,
                                               const Vector2 &variance,
                                               const BinUtility &binUtility) {
  auto [binX, binY, _] =
      binUtility.binTriple((Vector3() << position, 0).finished());
  Segmentizer::Bin2D bin = {static_cast<Segmentizer::Bin2D::value_type>(binX),
                            static_cast<Segmentizer::Bin2D::value_type>(binY)};
  Segmentizer::Segment2D segment = {position, position};
  double activation = 1;
  Cluster::Cell cell = {bin, segment, activation};

  Cluster cluster;
  cluster.sizeLoc0 = 1;
  cluster.sizeLoc1 = 1;
  cluster.channels = {cell};

  DigitizedParameters params;
  params.indices = {eBoundLoc0, eBoundLoc1};
  params.values = {position.x(), position.y()};
  params.variances = {variance.x(), variance.y()};
  params.cluster = {cluster};

  return params;
}

auto testDigitizedParametersWithTwoClusters(bool merge, const Vector2 &firstHit,
                                            const Vector2 &secondHit) {
  BinUtility binUtility;
  binUtility +=
      BinningData(BinningOption::open, BinningValue::binX, 20, -10.0f, 10.0f);
  binUtility +=
      BinningData(BinningOption::open, BinningValue::binY, 20, -10.0f, 10.0f);
  std::vector<Acts::BoundIndices> boundIndices = {eBoundLoc0, eBoundLoc1};
  double nsigma = 1;
  bool commonCorner = true;

  ModuleClusters moduleClusters(binUtility, boundIndices, merge, nsigma,
                                commonCorner);

  moduleClusters.add(makeDigitizationParameters(firstHit, {1, 1}, binUtility),
                     0);
  moduleClusters.add(makeDigitizationParameters(secondHit, {1, 1}, binUtility),
                     1);

  return moduleClusters.digitizedParameters();
}

}  // namespace

BOOST_AUTO_TEST_SUITE(DigitizationModuleClustersTests)

BOOST_AUTO_TEST_CASE(digitizedParameters_merging) {
  // overlapping hits are expected to be merged if turned on
  {
    auto result = testDigitizedParametersWithTwoClusters(true, {0, 0}, {0, 0});
    BOOST_CHECK_EQUAL(result.size(), 1);

    result = testDigitizedParametersWithTwoClusters(false, {0, 0}, {0, 0});
    BOOST_CHECK_EQUAL(result.size(), 2);
  }

  // non overlapping hits are not expected to be merged
  {
    auto result = testDigitizedParametersWithTwoClusters(true, {0, 0}, {5, 0});
    BOOST_CHECK_EQUAL(result.size(), 2);

    result = testDigitizedParametersWithTwoClusters(false, {0, 0}, {5, 0});
    BOOST_CHECK_EQUAL(result.size(), 2);
  }
}

BOOST_AUTO_TEST_SUITE_END()
