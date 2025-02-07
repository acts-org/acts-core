// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Utilities/BinAdjustmentVolume.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

namespace Acts::Test {

// Test Cylinder
BOOST_AUTO_TEST_CASE(BinAdjustmentVolume_Cylinder) {
  CylinderVolumeBounds bound(10, 50, 150, std::numbers::pi / 2., 0);
  BinUtility bu;
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisR);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisPhi);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisZ);

  BinUtility buAdjust = adjustBinUtility(bu, bound, Transform3::Identity());

  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].min, 10);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].max, 50);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].min,
                    -static_cast<float>(std::numbers::pi / 2.));
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].max,
                    static_cast<float>(std::numbers::pi / 2.));
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].min, -150);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].max, 150);
}

// Test Cutout Cylinder
BOOST_AUTO_TEST_CASE(BinAdjustmentVolume_CutoutCylinder) {
  CutoutCylinderVolumeBounds bound(10, 20, 50, 100, 15);
  BinUtility bu;
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisR);
  bu += BinUtility(1, 0, 1, Acts::closed, Acts::AxisDirection::AxisPhi);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisZ);

  BinUtility buAdjust = adjustBinUtility(bu, bound, Transform3::Identity());

  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].min, 10);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].max, 50);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].min, -std::numbers::pi_v<float>);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].max, std::numbers::pi_v<float>);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].min, -100);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].max, 100);
}

// Test Cuboid
BOOST_AUTO_TEST_CASE(BinAdjustmentVolume_Cuboid) {
  CuboidVolumeBounds bound(13, 23, 42);
  BinUtility bu;
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisX);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisY);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::AxisDirection::AxisZ);

  BinUtility buAdjust = adjustBinUtility(bu, bound, Transform3::Identity());

  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].min, -13);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].max, 13);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].min, -23);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].max, 23);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].min, -42);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].max, 42);
}

}  // namespace Acts::Test
