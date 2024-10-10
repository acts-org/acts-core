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
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <array>
#include <cmath>
#include <string>
#include <vector>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Geometry)

/// Unit tests for Polyderon construction & operator +=
BOOST_AUTO_TEST_CASE(ExtentTest) {
  std::vector<Vector3> vertices = {
      Vector3(15_mm, -3_mm, -10_mm), Vector3(18_mm, 0_mm, -10_mm),
      Vector3(15_mm, 3_mm, -10_mm),  Vector3(15_mm, -3_mm, 10_mm),
      Vector3(18_mm, 0_mm, 10_mm),   Vector3(15_mm, 3_mm, 10_mm)};

  // Create an Extent / without envelope
  Extent gExt;
  for (const auto& v : vertices) {
    gExt.extend(v);
  }

  double phiMin = std::atan2(-3_mm, 15_mm);
  double phiMax = std::atan2(3_mm, 15_mm);
  double rMin = std::hypot(15_mm, 3_mm);

  CHECK_CLOSE_ABS(gExt.min(BinningValue::binX), 15_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binX), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(BinningValue::binY), -3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binY), 3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(BinningValue::binZ), -10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binZ), 10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(BinningValue::binR), rMin, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binR), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(BinningValue::binPhi), phiMin, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binPhi), phiMax, 1e-6);

  // Call with histogram filling
  Extent gExtHist;
  for (const auto& v : vertices) {
    gExtHist.extend(v, {BinningValue::binX}, false, true);
  }
  const auto& vHist = gExtHist.valueHistograms();
  auto xVals = vHist[toUnderlying(BinningValue::binX)];

  BOOST_CHECK_EQUAL(xVals.size(), 6u);
  std::vector<ActsScalar> reference = {15_mm, 18_mm, 15_mm,
                                       15_mm, 18_mm, 15_mm};
  BOOST_CHECK(xVals == reference);

  // Call with ieterator range
  Extent gExtItr;
  gExtItr.extend(vertices.begin(), vertices.end());
  CHECK_CLOSE_ABS(gExtItr.min(BinningValue::binX), 15_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.max(BinningValue::binX), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.min(BinningValue::binY), -3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.max(BinningValue::binY), 3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.min(BinningValue::binZ), -10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.max(BinningValue::binZ), 10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.min(BinningValue::binR), rMin, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.max(BinningValue::binR), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.min(BinningValue::binPhi), phiMin, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.max(BinningValue::binPhi), phiMax, 1e-6);

  // Create a second Extent
  Extent gExtCopy;
  gExtCopy.extend(gExt);

  CHECK_CLOSE_ABS(gExtCopy.min(BinningValue::binX), 15_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.max(BinningValue::binX), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.min(BinningValue::binY), -3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.max(BinningValue::binY), 3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.min(BinningValue::binZ), -10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.max(BinningValue::binZ), 10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.min(BinningValue::binR), rMin, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.max(BinningValue::binR), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.min(BinningValue::binPhi), phiMin, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.max(BinningValue::binPhi), phiMax, 1e-6);

  // Check containment
  Extent unbound;
  BOOST_CHECK(unbound.contains(gExt));
  BOOST_CHECK(unbound.contains(gExtCopy));

  // Check application of an envelope on it
  ExtentEnvelope xEnvelopes = ExtentEnvelope::Zero();
  xEnvelopes[BinningValue::binX] = {1., 2.};

  // Take the extent and extend by an envelope
  Extent envelope(xEnvelopes);
  gExt.extend(envelope);
  // Changed ones
  CHECK_CLOSE_ABS(gExt.min(BinningValue::binX), 14_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binX), 20_mm, 1e-6);
  // Unchanged ones
  CHECK_CLOSE_ABS(gExt.min(BinningValue::binY), -3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binY), 3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(BinningValue::binZ), -10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binZ), 10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(BinningValue::binR), rMin, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binR), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(BinningValue::binPhi), phiMin, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binPhi), phiMax, 1e-6);

  // Fill it with envelope
  Extent gExtEnv(envelope);
  gExtEnv.extend(vertices.begin(), vertices.end());
  // Changed ones
  CHECK_CLOSE_ABS(gExtEnv.min(BinningValue::binX), 14_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtEnv.max(BinningValue::binX), 20_mm, 1e-6);

  // Check the set method
  gExt.set(BinningValue::binX, 2_mm, 8_mm);
  CHECK_CLOSE_ABS(gExt.min(BinningValue::binX), 2_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binX), 8_mm, 1e-6);

  // Radius can not go below 0
  gExt.set(BinningValue::binR, -2_mm, 18_mm);
  CHECK_CLOSE_ABS(gExt.min(BinningValue::binR), 0_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(BinningValue::binR), 18_mm, 1e-6);

  // Take an Extent and add a constraint
  Extent gExtConst;
  gExtConst.set(BinningValue::binR, 0., 5.);
  Extent gExtNonConst;
  BOOST_CHECK(!gExtNonConst.constrains(BinningValue::binR));
  gExtNonConst.addConstrain(gExtConst);
  BOOST_CHECK(gExtNonConst.constrains(BinningValue::binR));

  std::string tString = gExtConst.toString();
  BOOST_CHECK(!tString.empty());

  // Check single vertex containment
  Extent gExtVertexCheck;
  gExtVertexCheck.set(BinningValue::binR, 0., 5.);
  BOOST_CHECK(gExtVertexCheck.contains(Vector3(1., 0., 0.)));
  BOOST_CHECK(!gExtVertexCheck.contains(Vector3(6., 0., 0.)));
}

// Test that the constrains() check advances when the extend() method
// is used with a new binning type
BOOST_AUTO_TEST_CASE(ProtoSupportCaseTests) {
  std::vector<Vector3> vertices = {
      Vector3(15_mm, -3_mm, -10_mm), Vector3(18_mm, 0_mm, -10_mm),
      Vector3(15_mm, 3_mm, -10_mm),  Vector3(15_mm, -3_mm, 10_mm),
      Vector3(18_mm, 0_mm, 10_mm),   Vector3(15_mm, 3_mm, 10_mm)};

  Extent volumeExtent;
  volumeExtent.set(BinningValue::binZ, -300_mm, 300_mm);

  BOOST_CHECK(volumeExtent.constrains(BinningValue::binZ));
  BOOST_CHECK(!volumeExtent.constrains(BinningValue::binR));

  for (const auto& v : vertices) {
    volumeExtent.extend(v, {BinningValue::binR});
  }

  BOOST_CHECK(volumeExtent.constrains(BinningValue::binR));
}

BOOST_AUTO_TEST_CASE(DesignatedInitializers) {
  using enum BinningValue;
  ExtentEnvelope exp;
  exp[binX] = {1., 2.};
  exp[binEta] = {-1., 1.};

  ExtentEnvelope act{{.x = {1., 2.}, .eta = {-1., 1.}}};

  BOOST_CHECK(exp == act);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
