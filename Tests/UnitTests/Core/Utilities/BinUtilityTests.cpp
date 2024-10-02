// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace Acts::Test {

// OPEN - equidistant binning tests
BOOST_AUTO_TEST_CASE(BinUtility_equidistant_binning) {
  Vector3 xyzPosition(1.5, 2.5, 3.5);
  Vector3 edgePosition(0.5, 0.5, 0.5);

  // | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  BinUtility xUtil_eq(10, 0., 10., open, BinningValue::binX);
  BinUtility yUtil_eq(10, 0., 10., open, BinningValue::binY);
  BinUtility zUtil_eq(10, 0., 10., open, BinningValue::binZ);
  BOOST_CHECK_EQUAL(xUtil_eq.bins(), std::size_t{10});
  // make it 2-dim
  BinUtility xyUtil_eq(10, 0., 10., open, BinningValue::binX);
  xyUtil_eq += yUtil_eq;
  BOOST_CHECK_EQUAL(xyUtil_eq.bins(), 100u);
  // make it 3-dim
  BinUtility xyzUtil_eq(xyUtil_eq);
  xyzUtil_eq += zUtil_eq;
  BOOST_CHECK_EQUAL(xyzUtil_eq.bins(), 1000u);
  // check the dimensions
  BOOST_CHECK_EQUAL(xUtil_eq.dimensions(), 1u);
  BOOST_CHECK_EQUAL(xyUtil_eq.dimensions(), 2u);
  BOOST_CHECK_EQUAL(xyzUtil_eq.dimensions(), 3u);

  // check equality operator
  BinUtility xUtil_eq_copy(10, 0., 10., open, BinningValue::binX);
  BOOST_CHECK_EQUAL(xUtil_eq_copy, xUtil_eq);
  BOOST_CHECK_NE(yUtil_eq, xUtil_eq);

  // bin triples and clusters
  auto xTriple = xUtil_eq.binTriple(xyzPosition);
  auto xyTriple = xyUtil_eq.binTriple(xyzPosition);
  auto xyzTriple = xyzUtil_eq.binTriple(xyzPosition);

  BOOST_CHECK_EQUAL(xTriple[0], 1u);
  BOOST_CHECK_EQUAL(xTriple[1], 0u);
  BOOST_CHECK_EQUAL(xTriple[2], 0u);

  BOOST_CHECK_EQUAL(xyTriple[0], 1u);
  BOOST_CHECK_EQUAL(xyTriple[1], 2u);
  BOOST_CHECK_EQUAL(xyTriple[2], 0u);

  BOOST_CHECK_EQUAL(xyzTriple[0], 1u);
  BOOST_CHECK_EQUAL(xyzTriple[1], 2u);
  BOOST_CHECK_EQUAL(xyzTriple[2], 3u);
}

// OPEN - equidistant binning tests
BOOST_AUTO_TEST_CASE(BinUtility_arbitrary_binning) {
  std::vector<float> bvalues = {-5., 0., 1., 1.1, 8.};
  BinUtility xUtil(bvalues, Acts::open, Acts::BinningValue::binX);

  // Underflow
  BOOST_CHECK_EQUAL(xUtil.bin(Vector3(-6., 0., 0.)), 0u);
  // Bin 0
  BOOST_CHECK_EQUAL(xUtil.bin(Vector3(-4., 0., 0.)), 0u);
  // Bin 1
  BOOST_CHECK_EQUAL(xUtil.bin(Vector3(0.5, 0., 0.)), 1u);
  // Bin 2
  BOOST_CHECK_EQUAL(xUtil.bin(Vector3(1.05, 0., 0.)), 2u);
  // Bin 3
  BOOST_CHECK_EQUAL(xUtil.bin(Vector3(4., 0., 0.)), 3u);
  // Overflow
  BOOST_CHECK_EQUAL(xUtil.bin(Vector3(9., 0., 0.)), 3u);
}

// OPEN - local to global transform test
BOOST_AUTO_TEST_CASE(BinUtility_transform) {
  Transform3 transform_LtoG = Transform3::Identity();
  transform_LtoG = transform_LtoG * Translation3(0., 0., -50);
  transform_LtoG = transform_LtoG * AngleAxis3(M_PI / 4, Vector3(0, 0, 1));

  Transform3 transform_GtoL = transform_LtoG.inverse();

  BinUtility rUtil(10, 0., 100., open, BinningValue::binR);
  BinUtility phiUtil(10, -M_PI, M_PI, closed, BinningValue::binPhi);
  BinUtility zUtil(10, -100., 100., open, BinningValue::binZ);

  BinUtility noTranform;
  noTranform += rUtil;
  noTranform += phiUtil;
  noTranform += zUtil;

  BinUtility withTranform(transform_LtoG);
  withTranform += rUtil;
  withTranform += phiUtil;
  withTranform += zUtil;

  Vector3 pos1(0, 0, 0);
  Vector3 pos2(60, 0, 0);
  Vector3 pos3(34, M_PI / 2, 0);
  Vector3 pos4(0, 0, -80);
  Vector3 pos5(80, -M_PI / 4, 50);

  for (int i = 0; i < 3; i++) {
    BOOST_CHECK_EQUAL(withTranform.bin(pos1, i),
                      noTranform.bin(transform_GtoL * pos1, i));
    BOOST_CHECK_EQUAL(withTranform.bin(pos2, i),
                      noTranform.bin(transform_GtoL * pos2, i));
    BOOST_CHECK_EQUAL(withTranform.bin(pos3, i),
                      noTranform.bin(transform_GtoL * pos3, i));
    BOOST_CHECK_EQUAL(withTranform.bin(pos4, i),
                      noTranform.bin(transform_GtoL * pos4, i));
    BOOST_CHECK_EQUAL(withTranform.bin(pos5, i),
                      noTranform.bin(transform_GtoL * pos5, i));
  }
}

}  // namespace Acts::Test
