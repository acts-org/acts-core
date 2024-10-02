// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
#include <vector>

namespace Acts::Test {

// Test Cylinder
BOOST_AUTO_TEST_CASE(BinAdjustmentVolume_Cylinder) {
  CylinderVolumeBounds bound(10, 50, 150, M_PI / 2, 0);
  BinUtility bu;
  bu += BinUtility(1, 0, 1, Acts::open, Acts::BinningValue::binR);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::BinningValue::binPhi);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::BinningValue::binZ);

  BinUtility buAdjust = adjustBinUtility(bu, bound, Transform3::Identity());

  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].min, 10);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].max, 50);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].min, float{-M_PI / 2});
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].max, float{M_PI / 2});
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].min, -150);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].max, 150);
}

// Test Cutout Cylinder
BOOST_AUTO_TEST_CASE(BinAdjustmentVolume_CutoutCylinder) {
  CutoutCylinderVolumeBounds bound(10, 20, 50, 100, 15);
  BinUtility bu;
  bu += BinUtility(1, 0, 1, Acts::open, Acts::BinningValue::binR);
  bu += BinUtility(1, 0, 1, Acts::closed, Acts::BinningValue::binPhi);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::BinningValue::binZ);

  BinUtility buAdjust = adjustBinUtility(bu, bound, Transform3::Identity());

  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].min, 10);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].max, 50);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].min, float{-M_PI});
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].max, float{M_PI});
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].min, -100);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].max, 100);
}

// Test Cuboid
BOOST_AUTO_TEST_CASE(BinAdjustmentVolume_Cuboid) {
  CuboidVolumeBounds bound(13, 23, 42);
  BinUtility bu;
  bu += BinUtility(1, 0, 1, Acts::open, Acts::BinningValue::binX);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::BinningValue::binY);
  bu += BinUtility(1, 0, 1, Acts::open, Acts::BinningValue::binZ);

  BinUtility buAdjust = adjustBinUtility(bu, bound, Transform3::Identity());

  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].min, -13);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[0].max, 13);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].min, -23);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[1].max, 23);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].min, -42);
  BOOST_CHECK_EQUAL(buAdjust.binningData()[2].max, 42);
}

}  // namespace Acts::Test
