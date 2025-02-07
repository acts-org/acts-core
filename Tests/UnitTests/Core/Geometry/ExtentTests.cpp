// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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

  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisX), 15_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisX), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisY), -3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisY), 3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisZ), -10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisZ), 10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisR), rMin, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisR), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisPhi), phiMin, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisPhi), phiMax, 1e-6);

  // Call with histogram filling
  Extent gExtHist;
  for (const auto& v : vertices) {
    gExtHist.extend(v, {AxisDirection::AxisX}, false, true);
  }
  const auto& vHist = gExtHist.valueHistograms();
  auto xVals = vHist[toUnderlying(AxisDirection::AxisX)];

  BOOST_CHECK_EQUAL(xVals.size(), 6u);
  std::vector<double> reference = {15_mm, 18_mm, 15_mm, 15_mm, 18_mm, 15_mm};
  BOOST_CHECK(xVals == reference);

  // Call with ieterator range
  Extent gExtItr;
  gExtItr.extend(vertices.begin(), vertices.end());
  CHECK_CLOSE_ABS(gExtItr.min(AxisDirection::AxisX), 15_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.max(AxisDirection::AxisX), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.min(AxisDirection::AxisY), -3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.max(AxisDirection::AxisY), 3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.min(AxisDirection::AxisZ), -10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.max(AxisDirection::AxisZ), 10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.min(AxisDirection::AxisR), rMin, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.max(AxisDirection::AxisR), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.min(AxisDirection::AxisPhi), phiMin, 1e-6);
  CHECK_CLOSE_ABS(gExtItr.max(AxisDirection::AxisPhi), phiMax, 1e-6);

  // Create a second Extent
  Extent gExtCopy;
  gExtCopy.extend(gExt);

  CHECK_CLOSE_ABS(gExtCopy.min(AxisDirection::AxisX), 15_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.max(AxisDirection::AxisX), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.min(AxisDirection::AxisY), -3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.max(AxisDirection::AxisY), 3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.min(AxisDirection::AxisZ), -10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.max(AxisDirection::AxisZ), 10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.min(AxisDirection::AxisR), rMin, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.max(AxisDirection::AxisR), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.min(AxisDirection::AxisPhi), phiMin, 1e-6);
  CHECK_CLOSE_ABS(gExtCopy.max(AxisDirection::AxisPhi), phiMax, 1e-6);

  // Check containment
  Extent unbound;
  BOOST_CHECK(unbound.contains(gExt));
  BOOST_CHECK(unbound.contains(gExtCopy));

  // Check application of an envelope on it
  ExtentEnvelope xEnvelopes = ExtentEnvelope::Zero();
  xEnvelopes[AxisDirection::AxisX] = {1., 2.};

  // Take the extent and extend by an envelope
  Extent envelope(xEnvelopes);
  gExt.extend(envelope);
  // Changed ones
  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisX), 14_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisX), 20_mm, 1e-6);
  // Unchanged ones
  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisY), -3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisY), 3_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisZ), -10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisZ), 10_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisR), rMin, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisR), 18_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisPhi), phiMin, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisPhi), phiMax, 1e-6);

  // Fill it with envelope
  Extent gExtEnv(envelope);
  gExtEnv.extend(vertices.begin(), vertices.end());
  // Changed ones
  CHECK_CLOSE_ABS(gExtEnv.min(AxisDirection::AxisX), 14_mm, 1e-6);
  CHECK_CLOSE_ABS(gExtEnv.max(AxisDirection::AxisX), 20_mm, 1e-6);

  // Check the set method
  gExt.set(AxisDirection::AxisX, 2_mm, 8_mm);
  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisX), 2_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisX), 8_mm, 1e-6);

  // Radius can not go below 0
  gExt.set(AxisDirection::AxisR, -2_mm, 18_mm);
  CHECK_CLOSE_ABS(gExt.min(AxisDirection::AxisR), 0_mm, 1e-6);
  CHECK_CLOSE_ABS(gExt.max(AxisDirection::AxisR), 18_mm, 1e-6);

  // Take an Extent and add a constraint
  Extent gExtConst;
  gExtConst.set(AxisDirection::AxisR, 0., 5.);
  Extent gExtNonConst;
  BOOST_CHECK(!gExtNonConst.constrains(AxisDirection::AxisR));
  gExtNonConst.addConstrain(gExtConst);
  BOOST_CHECK(gExtNonConst.constrains(AxisDirection::AxisR));

  std::string tString = gExtConst.toString();
  BOOST_CHECK(!tString.empty());

  // Check single vertex containment
  Extent gExtVertexCheck;
  gExtVertexCheck.set(AxisDirection::AxisR, 0., 5.);
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
  volumeExtent.set(AxisDirection::AxisZ, -300_mm, 300_mm);

  BOOST_CHECK(volumeExtent.constrains(AxisDirection::AxisZ));
  BOOST_CHECK(!volumeExtent.constrains(AxisDirection::AxisR));

  for (const auto& v : vertices) {
    volumeExtent.extend(v, {AxisDirection::AxisR});
  }

  BOOST_CHECK(volumeExtent.constrains(AxisDirection::AxisR));
}

BOOST_AUTO_TEST_CASE(DesignatedInitializers) {
  using enum AxisDirection;
  ExtentEnvelope exp;
  exp[AxisX] = {1., 2.};
  exp[AxisEta] = {-1., 1.};

  ExtentEnvelope act{{.x = {1., 2.}, .eta = {-1., 1.}}};

  BOOST_CHECK(exp == act);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
