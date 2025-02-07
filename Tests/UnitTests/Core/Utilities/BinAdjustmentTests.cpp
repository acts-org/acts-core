// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Utilities/BinAdjustment.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

namespace Acts::Test {

// Test Radial
BOOST_AUTO_TEST_CASE(BinAdjustment_Radial) {
  RadialBounds bound(50, 75, std::numbers::pi, 0);
  BinUtility bu;
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisR);
  bu += BinUtility(1, 0, 1, Acts::closed, Acts::AxisDirection::AxisPhi);

  BinUtility buAdjust = adjustBinUtility(bu, bound, Transform3::Identity());

  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].min, 50);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].max, 75);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].min, -std::numbers::pi_v<float>);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].max, std::numbers::pi_v<float>);
}

// Test Cylinder
BOOST_AUTO_TEST_CASE(BinAdjustment_Cylinder) {
  CylinderBounds bound(25, 50, std::numbers::pi / 4, 0);
  BinUtility bu;
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisPhi);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisZ);

  BinUtility buAdjust = adjustBinUtility(bu, bound, Transform3::Identity());

  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].min,
                    -static_cast<float>(std::numbers::pi / 4.));
  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].max,
                    static_cast<float>(std::numbers::pi / 4.));
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].min, -50);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].max, 50);
}

// Test Rectangule
BOOST_AUTO_TEST_CASE(BinAdjustment_Rectangle) {
  RectangleBounds bound(20, 30);
  BinUtility bu;
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisX);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisY);

  BinUtility buAdjust = adjustBinUtility(bu, bound, Transform3::Identity());

  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].min, -20);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].max, 20);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].min, -30);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].max, 30);
}

// Test Trapezoid
BOOST_AUTO_TEST_CASE(BinAdjustment_Trapezoid) {
  TrapezoidBounds bound(5, 15, 30);
  BinUtility bu;
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisX);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisY);

  BinUtility buAdjust = adjustBinUtility(bu, bound, Transform3::Identity());

  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].min, -15);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].max, 15);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].min, -30);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].max, 30);
}

}  // namespace Acts::Test
