// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Visualization/EventDataView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/PlyVisualization3D.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "PrimitivesView3DBase.hpp"
#include "Visualization3DTester.hpp"

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Visualization)

/// The tests in this section are regression tests only in order
/// to catch any unexpected changes in the output format.
///
BOOST_AUTO_TEST_CASE(Visualization3DHelpers) {
  // No correlation, fully symmetric
  SquareMatrix2 covariance;
  covariance << 4., 0., 0., 4.;
  auto decops = Acts::EventDataView3D::decomposeCovariance(covariance);
  BOOST_CHECK_EQUAL(decops[0], 4.);
  BOOST_CHECK_EQUAL(decops[1], 4.);
  BOOST_CHECK_EQUAL(decops[2], 0.);

  // Fully positively correlated
  covariance.setZero();
  covariance << 4., 4., 4., 4.;
  decops = Acts::EventDataView3D::decomposeCovariance(covariance);
  BOOST_CHECK_EQUAL(decops[0], 8.);
  BOOST_CHECK_EQUAL(decops[1], 0.);
  CHECK_CLOSE_ABS(decops[2], M_PI / 4, 0.0001);

  // Fully negatively correlated
  covariance.setZero();
  covariance << 4., -4., -4., 4.;
  decops = Acts::EventDataView3D::decomposeCovariance(covariance);
  BOOST_CHECK_EQUAL(decops[0], 8.);
  BOOST_CHECK_EQUAL(decops[1], 0.);
  CHECK_CLOSE_ABS(decops[2], 3 * M_PI / 4, 0.0001);

  // Correlation coefficient 0.5 (off-diagonal: 3*2*0.5)
  covariance.setZero();
  covariance << 4., 2., 2., 4.;
  decops = Acts::EventDataView3D::decomposeCovariance(covariance);
  BOOST_CHECK_EQUAL(decops[0], 6.);
  BOOST_CHECK_EQUAL(decops[1], 2.);
  CHECK_CLOSE_ABS(decops[2], M_PI / 4, 0.0001);

  // Correlation coefficient -0.5 & different diagonal (off-diagonal: 3*2*0.5)
  covariance.setZero();
  covariance << 9., -3., -3, 4.;
  decops = Acts::EventDataView3D::decomposeCovariance(covariance);
  CHECK_CLOSE_ABS(decops[0], 10.4051, 0.0001);
  CHECK_CLOSE_ABS(decops[1], 2.59488, 0.0001);
  CHECK_CLOSE_ABS(decops[2], 2.70356, 0.0001);
}

BOOST_AUTO_TEST_CASE(PrimitivesView3DObj) {
  ObjVisualization3D obj;
  auto objTest = PrimitivesView3DTest::run(obj);
  auto objErrors = testObjString(objTest);
  BOOST_CHECK(objErrors.empty());
  for (const auto& objerr : objErrors) {
    std::cout << objerr << std::endl;
  }
}

BOOST_AUTO_TEST_CASE(PrimitivesView3DPly) {
  PlyVisualization3D ply;
  auto plyTest = PrimitivesView3DTest::run(ply);
  auto plyErrors = testPlyString(plyTest);
  BOOST_CHECK(plyErrors.empty());
  for (const auto& plyerr : plyErrors) {
    std::cout << plyerr << std::endl;
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
