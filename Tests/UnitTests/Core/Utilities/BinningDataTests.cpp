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
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace Acts::Test {

// the test positions in 3D
Vector3 xyzPosition(0.5, 1.5, 2.5);
Vector3 xyzPositionOutside(30., -30., 200.);
Vector3 phi0Position(0.5, 0., 2.5);
Vector3 phiPihPosition(0., 1.5, 2.5);
Vector3 eta0Position(0.5, 1.5, 0.);
// the test positions in 2D
Vector2 xyPosition(0.5, 1.5);
Vector2 rphizPosition(0.1, 2.5);
Vector2 rphiPosition(3.5, M_PI / 8.);

// the binnings - equidistant
// x/y/zData
// bin boundaries
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
BinningData xData_eq(open, BinningValue::binX, 10, 0., 10.);
BinningData yData_eq(open, BinningValue::binY, 10, 0., 10.);
BinningData zData_eq(open, BinningValue::binZ, 10, 0., 10.);
// r/phi/rphiData
// bin boundaries
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
BinningData rData_eq(open, BinningValue::binR, 10, 0., 10.);
// bin boundaries
// > -M_PI | -3/5 M_PI | -1/5 M_PI | 1/5 M_PI | 3/5 M_PI | M_PI <
BinningData phiData_eq(closed, BinningValue::binPhi, 5, -M_PI, M_PI);
// BinningData rPhiData_eq(closed, BinningValue::binRPhi, 5, -M_PI, M_PI);
// h/etaData
// bin boundaries
// | 0 | 2 | 4 | 6 | 8 | 10 |
// BinningData hData_eq(open, BinningValue::binH, 5, 0., 10.);
// | -2.5 | -1.5 | -0.5 | 0.5 | 1.5 | 2.5 |
BinningData etaData_eq(open, BinningValue::binEta, 5, -2.5, 2.5);

// Fest equality operator
BinningData xData_eq_copy(open, BinningValue::binX, 10, 0., 10.);

// the binnings - arbitrary
std::vector<float> values = {0., 1., 2., 3., 4., 10.};
// bin boundaries
// | 0 | 1 | 2 | 3 | 4 | 10 |
BinningData xData_arb(open, BinningValue::binX, values);
BinningData yData_arb(open, BinningValue::binY, values);
// | -M_PI |  -2 |  -1 |  1 |  2 |  M_PI |
std::vector<float> phiValues = {-M_PI, -2., -1., 1., 2., M_PI};
BinningData phiData_arb(closed, BinningValue::binPhi, phiValues);

// the binnings - arbitrary when switching to binary search - for boundary
// sizes >= 50
std::size_t nBins_binary = 59;
double valueMin = 0.;
double phiMin = -M_PI;
double delta = 0.5;
double phiDelta = 0.1064;

// the binning - substructure
std::vector<float> sstr = {0., 1., 1.5, 2., 3.};
// multiplicative
auto xData_sstr_mult =
    std::make_unique<const BinningData>(open, BinningValue::binX, sstr);
// | 0 | 1 | 1.5 | 2 |  3 | 4 | 4.5 | 5 | 6 | 7 | 7.5 | 8 | 9 |
BinningData xData_mult(open, BinningValue::binX, 3, 0., 9.,
                       std::move(xData_sstr_mult));
/// additive
// | 0 | 1 | 1.5 | 2 |  3 | 4 | 5 |
std::vector<float> main_sstr = {0., 3., 4., 5.};
auto xData_sstr_add =
    std::make_unique<const BinningData>(open, BinningValue::binX, sstr);
BinningData xData_add(open, BinningValue::binX, main_sstr,
                      std::move(xData_sstr_add));

// enum BinningValue { BinningValue::binX, BinningValue::binY,
// BinningValue::binZ, BinningValue::binR, BinningValue::binPhi,
// BinningValue::binRPhi, BinningValue::binH, BinningValue::binEta }
//
// test the different binning values
BOOST_AUTO_TEST_CASE(BinningData_BinningValue) {
  // the binnings - arbitrary when switching to binary search - for boundary
  // sizes >= 50
  std::vector<float> values_binary;
  std::vector<float> phiValues_binary;
  for (std::size_t i = 0; i <= nBins_binary; i++) {
    values_binary.push_back(valueMin + i * delta);
    phiValues_binary.push_back(phiMin + i * phiDelta);
  }
  // bin boundaries when switching to binary search - for boundary sizes >= 50
  BinningData xData_arb_binary(open, BinningValue::binX, values_binary);
  BinningData phiData_arb_binary(closed, BinningValue::binPhi,
                                 phiValues_binary);
  /// x/y/zData
  /// check the global position requests
  // | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  BOOST_CHECK_EQUAL(xData_eq.bins(), std::size_t{10});
  // | 0 | 1 | 2 | 3 | 4 | 10 |
  BOOST_CHECK_EQUAL(xData_arb.bins(), std::size_t{5});
  // | 0 | 1 | 1.5 | 2 | 3 | 4 | 4.5 | 5 | 6 | 7 | 7.5 | 8 | 9 |
  BOOST_CHECK_EQUAL(xData_mult.bins(), std::size_t{12});
  // | 0 | 1 | 1.5 | 2 |  3 | 4 | 5 |
  BOOST_CHECK_EQUAL(xData_add.bins(), std::size_t{6});
  BOOST_CHECK_EQUAL(xData_arb_binary.bins(), nBins_binary);

  BOOST_CHECK(xData_eq_copy == xData_eq_copy);
  BOOST_CHECK(!(xData_eq == yData_eq));

  /// check the global position requests
  BOOST_CHECK_EQUAL(xData_eq.value(xyzPosition), 0.5);
  BOOST_CHECK_EQUAL(yData_eq.value(xyzPosition), 1.5);
  BOOST_CHECK_EQUAL(zData_eq.value(xyzPosition), 2.5);
  BOOST_CHECK_EQUAL(xData_arb.value(xyzPosition), 0.5);
  BOOST_CHECK_EQUAL(xData_mult.value(xyzPosition), 0.5);
  BOOST_CHECK_EQUAL(xData_add.value(xyzPosition), 0.5);
  BOOST_CHECK_EQUAL(xData_arb_binary.value(xyzPosition), 0.5);

  /// check the local position requests
  BOOST_CHECK_EQUAL(xData_eq.value(xyPosition), 0.5);
  BOOST_CHECK_EQUAL(yData_eq.value(xyPosition), 1.5);
  BOOST_CHECK_EQUAL(zData_eq.value(rphizPosition), 2.5);
  BOOST_CHECK_EQUAL(xData_arb.value(xyPosition), 0.5);
  BOOST_CHECK_EQUAL(xData_mult.value(xyPosition), 0.5);
  BOOST_CHECK_EQUAL(xData_add.value(xyPosition), 0.5);
  BOOST_CHECK_EQUAL(xData_arb_binary.value(xyPosition), 0.5);

  // r/phi/rphiData
  CHECK_CLOSE_REL(rData_eq.value(xyzPosition), std::hypot(0.5, 1.5), 1e-5);
  BOOST_CHECK_EQUAL(rData_eq.value(rphiPosition), 3.5);

  CHECK_SMALL(phiData_eq.value(phi0Position), 1e-6 * M_PI);
  CHECK_CLOSE_REL(phiData_eq.value(phiPihPosition), M_PI / 2, 1e-5);

  BOOST_CHECK_EQUAL(phiData_eq.bins(), std::size_t{5});
  BOOST_CHECK_EQUAL(phiData_arb.bins(), std::size_t{5});
  BOOST_CHECK_EQUAL(phiData_arb_binary.bins(), nBins_binary);

  // h/etaData
  CHECK_SMALL(etaData_eq.value(eta0Position), 1e-5);
}

// test bin values
BOOST_AUTO_TEST_CASE(BinningData_bins) {
  // the binnings - arbitrary when switching to binary search - for boundary
  // sizes >= 50
  std::vector<float> values_binary;
  std::vector<float> phiValues_binary;
  for (std::size_t i = 0; i <= nBins_binary; i++) {
    values_binary.push_back(valueMin + i * delta);
    phiValues_binary.push_back(phiMin + i * phiDelta);
  }
  // bin boundaries when switching to binary search - for boundary sizes >= 50
  BinningData xData_arb_binary(open, BinningValue::binX, values_binary);
  BinningData phiData_arb_binary(closed, BinningValue::binPhi,
                                 phiValues_binary);
  /// x/y/zData
  /// check the global position requests
  // | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  BOOST_CHECK_EQUAL(xData_eq.searchGlobal(xyzPosition), std::size_t{0});
  BOOST_CHECK_EQUAL(yData_eq.searchGlobal(xyzPosition), std::size_t{1});
  BOOST_CHECK_EQUAL(zData_eq.searchGlobal(xyzPosition), std::size_t{2});
  // | 0 | 1 | 2 | 3 | 4 | 10 |
  BOOST_CHECK_EQUAL(xData_arb.searchGlobal(xyzPosition), std::size_t{0});
  BOOST_CHECK_EQUAL(xData_arb.search(6.), std::size_t{4});
  BOOST_CHECK_EQUAL(xData_arb_binary.searchGlobal(xyzPosition), std::size_t{0});
  BOOST_CHECK_EQUAL(xData_arb_binary.search(50.), (nBins_binary - 1));
  // | 0 | 1 | 1.5 | 2 |  3 | 4 | 5 |
  BOOST_CHECK_EQUAL(xData_add.searchGlobal(xyzPosition), std::size_t{0});
  BOOST_CHECK_EQUAL(xData_add.searchGlobal(xyzPosition), std::size_t{0});
  BOOST_CHECK_EQUAL(xData_add.search(0.2), std::size_t{0});
  BOOST_CHECK_EQUAL(xData_add.search(1.2), std::size_t{1});
  BOOST_CHECK_EQUAL(xData_add.search(1.7), std::size_t{2});
  BOOST_CHECK_EQUAL(xData_add.search(2.5), std::size_t{3});
  BOOST_CHECK_EQUAL(xData_add.search(3.5), std::size_t{4});
  BOOST_CHECK_EQUAL(xData_add.search(4.2), std::size_t{5});
  BOOST_CHECK_EQUAL(xData_add.search(7.), std::size_t{5});
  // | 0 | 1 | 1.5 | 2 | 3 | 4 | 4.5 | 5 | 6 | 7 | 7.5 | 8 | 9 |
  BOOST_CHECK_EQUAL(xData_mult.searchGlobal(xyzPosition), std::size_t{0});
  BOOST_CHECK_EQUAL(xData_mult.search(0.2), std::size_t{0});
  BOOST_CHECK_EQUAL(xData_mult.search(1.2), std::size_t{1});
  BOOST_CHECK_EQUAL(xData_mult.search(1.7), std::size_t{2});
  BOOST_CHECK_EQUAL(xData_mult.search(2.5), std::size_t{3});
  BOOST_CHECK_EQUAL(xData_mult.search(3.5), std::size_t{4});
  BOOST_CHECK_EQUAL(xData_mult.search(4.2), std::size_t{5});
  BOOST_CHECK_EQUAL(xData_mult.search(4.7), std::size_t{6});
  BOOST_CHECK_EQUAL(xData_mult.search(5.7), std::size_t{7});
  BOOST_CHECK_EQUAL(xData_mult.search(6.5), std::size_t{8});
  BOOST_CHECK_EQUAL(xData_mult.search(7.2), std::size_t{9});
  BOOST_CHECK_EQUAL(xData_mult.search(7.7), std::size_t{10});
  BOOST_CHECK_EQUAL(xData_mult.search(8.1), std::size_t{11});

  /// check the local position requests
  BOOST_CHECK_EQUAL(xData_eq.searchLocal(xyPosition), std::size_t{0});
  BOOST_CHECK_EQUAL(yData_eq.searchLocal(xyPosition), std::size_t{1});
  BOOST_CHECK_EQUAL(zData_eq.searchLocal(rphizPosition), std::size_t{2});
  BOOST_CHECK_EQUAL(xData_arb.searchLocal(xyPosition), std::size_t{0});
  BOOST_CHECK_EQUAL(xData_arb_binary.searchLocal(xyPosition), std::size_t{0});

  // r/phi/rphiData
  BOOST_CHECK_EQUAL(rData_eq.searchGlobal(xyzPosition), std::size_t{1});
  BOOST_CHECK_EQUAL(rData_eq.searchLocal(rphiPosition), std::size_t{3});
  BOOST_CHECK_EQUAL(phiData_eq.searchGlobal(phi0Position), std::size_t{2});
  BOOST_CHECK_EQUAL(phiData_eq.searchGlobal(phiPihPosition), std::size_t{3});
  BOOST_CHECK_EQUAL(phiData_arb_binary.search(M_PI), std::size_t{0});

  // h/etaData
  BOOST_CHECK_EQUAL(etaData_eq.searchGlobal(eta0Position), std::size_t{2});
}

// test inside/outside
BOOST_AUTO_TEST_CASE(BinningData_inside_outside) {
  // the binnings - arbitrary when switching to binary search - for boundary
  // sizes >= 50
  std::vector<float> values_binary;
  std::vector<float> phiValues_binary;
  for (std::size_t i = 0; i <= nBins_binary; i++) {
    values_binary.push_back(valueMin + i * delta);
    phiValues_binary.push_back(phiMin + i * phiDelta);
  }
  // bin boundaries when switching to binary search - for boundary sizes >= 50
  BinningData xData_arb_binary(open, BinningValue::binX, values_binary);
  BinningData phiData_arb_binary(closed, BinningValue::binPhi,
                                 phiValues_binary);
  // check the global inside
  BOOST_CHECK_EQUAL(xData_eq.inside(xyzPosition), true);
  BOOST_CHECK_EQUAL(yData_eq.inside(xyzPosition), true);
  BOOST_CHECK_EQUAL(zData_eq.inside(xyzPosition), true);
  BOOST_CHECK_EQUAL(xData_arb.inside(xyzPosition), true);
  BOOST_CHECK_EQUAL(xData_add.inside(xyzPosition), true);
  BOOST_CHECK_EQUAL(xData_mult.inside(xyzPosition), true);
  BOOST_CHECK_EQUAL(xData_arb_binary.inside(xyzPosition), true);

  // check the global outside
  BOOST_CHECK_EQUAL(xData_eq.inside(xyzPositionOutside), false);
  BOOST_CHECK_EQUAL(yData_eq.inside(xyzPositionOutside), false);
  BOOST_CHECK_EQUAL(zData_eq.inside(xyzPositionOutside), false);
  BOOST_CHECK_EQUAL(xData_arb.inside(xyzPositionOutside), false);
  BOOST_CHECK_EQUAL(xData_add.inside(xyzPositionOutside), false);
  BOOST_CHECK_EQUAL(xData_mult.inside(xyzPositionOutside), false);
  BOOST_CHECK_EQUAL(xData_arb_binary.inside(xyzPositionOutside), false);

  // cthe local inside
  BOOST_CHECK_EQUAL(xData_eq.inside(xyPosition), true);
  BOOST_CHECK_EQUAL(yData_eq.inside(xyPosition), true);
  BOOST_CHECK_EQUAL(zData_eq.inside(rphizPosition), true);

  // r/phi/rphiData inside
  BOOST_CHECK_EQUAL(phiData_eq.inside(phi0Position), true);
  BOOST_CHECK_EQUAL(phiData_eq.inside(phiPihPosition), true);
  //// h/etaData
  BOOST_CHECK_EQUAL(etaData_eq.inside(eta0Position), true);
}

// test open/close
BOOST_AUTO_TEST_CASE(BinningData_open_close) {
  // the binnings - arbitrary when switching to binary search - for boundary
  // sizes >= 50
  std::vector<float> values_binary;
  std::vector<float> phiValues_binary;
  for (std::size_t i = 0; i <= nBins_binary; i++) {
    values_binary.push_back(valueMin + i * delta);
    phiValues_binary.push_back(phiMin + i * phiDelta);
  }
  // bin boundaries when switching to binary search - for boundary sizes >= 50
  BinningData xData_arb_binary(open, BinningValue::binX, values_binary);
  BinningData phiData_arb_binary(closed, BinningValue::binPhi,
                                 phiValues_binary);
  // open values
  BOOST_CHECK_EQUAL(xData_eq.searchGlobal(xyzPositionOutside), std::size_t{9});
  BOOST_CHECK_EQUAL(yData_eq.searchGlobal(xyzPositionOutside), std::size_t{0});
  BOOST_CHECK_EQUAL(zData_eq.searchGlobal(xyzPositionOutside), std::size_t{9});
  BOOST_CHECK_EQUAL(xData_arb.searchGlobal(xyzPositionOutside) + 1,
                    xData_arb.bins());
  BOOST_CHECK_EQUAL(xData_arb_binary.searchGlobal(xyzPositionOutside) + 1,
                    xData_arb_binary.bins());
  BOOST_CHECK_EQUAL(yData_arb.searchGlobal(xyzPositionOutside), std::size_t{0});

  // closed values
  BOOST_CHECK_EQUAL(phiData_eq.search(-4.), std::size_t{4});
  BOOST_CHECK_EQUAL(phiData_eq.search(4.), std::size_t{0});
  BOOST_CHECK_EQUAL(phiData_arb.search(-4.), std::size_t{4});
  BOOST_CHECK_EQUAL(phiData_arb.search(4.), std::size_t{0});
  BOOST_CHECK_EQUAL(phiData_arb_binary.search(-4.), (nBins_binary - 1));
  BOOST_CHECK_EQUAL(phiData_arb_binary.search(4.), std::size_t{0});
}

// test boundaries
BOOST_AUTO_TEST_CASE(BinningData_boundaries) {
  // open values
  std::vector<float> boundaries = {0., 1., 2., 3., 4., 5., 6., 7., 8., 9., 10.};
  BOOST_CHECK_EQUAL_COLLECTIONS(xData_eq.boundaries().begin(),
                                xData_eq.boundaries().end(), boundaries.begin(),
                                boundaries.end());

  float phiStep = M_PI * 2. / 5.;
  std::vector<float> phiBoundaries_eq = {
      -M_PI,
      static_cast<float>(-M_PI + 1 * phiStep),
      static_cast<float>(-M_PI + 2 * phiStep),
      static_cast<float>(-M_PI + 3 * phiStep),
      static_cast<float>(-M_PI + 4 * phiStep),
      static_cast<float>(-M_PI + 5 * phiStep)};
  CHECK_CLOSE_REL(phiData_eq.boundaries(), phiBoundaries_eq, 1e-5);
}

// test bin center values
// test boundaries
BOOST_AUTO_TEST_CASE(BinningData_bincenter) {
  // the binnings - arbitrary when switching to binary search - for boundary
  // sizes >= 50
  std::vector<float> values_binary;
  std::vector<float> phiValues_binary;
  for (std::size_t i = 0; i <= nBins_binary; i++) {
    values_binary.push_back(valueMin + i * delta);
    phiValues_binary.push_back(phiMin + i * phiDelta);
  }
  // bin boundaries when switching to binary search - for boundary sizes >= 50
  BinningData xData_arb_binary(open, BinningValue::binX, values_binary);
  BinningData phiData_arb_binary(closed, BinningValue::binPhi,
                                 phiValues_binary);
  /// check the global position requests
  // | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  BOOST_CHECK_EQUAL(xData_eq.center(3), 3.5);
  // | 0 | 1 | 2 | 3 | 4 | 10 |
  BOOST_CHECK_EQUAL(xData_arb.center(4), 7.);
  // | 0 | 1 | 1.5 | 2 |  3 | 4 | 5 |
  BOOST_CHECK_EQUAL(xData_add.center(0), 0.5);
  BOOST_CHECK_EQUAL(xData_add.center(1), 1.25);
  BOOST_CHECK_EQUAL(xData_add.center(4), 3.5);
  // | 0 | 1 | 1.5 | 2 | 3 | 4 | 4.5 | 5 | 6 | 7 | 7.5 | 8 | 9 |
  BOOST_CHECK_EQUAL(xData_mult.center(0), 0.5);
  BOOST_CHECK_EQUAL(xData_mult.center(1), 1.25);
  BOOST_CHECK_EQUAL(xData_mult.center(4), 3.5);
  BOOST_CHECK_EQUAL(xData_mult.center(10), 7.75);
  BOOST_CHECK_EQUAL(xData_mult.center(11), 8.5);

  BOOST_CHECK_EQUAL(xData_arb_binary.center(0), 0.5 * delta);

  // open values
  std::vector<float> center = {0.5, 1.5, 2.5, 3.5, 4.5,
                               5.5, 6.5, 7.5, 8.5, 9.5};
  for (std::size_t ib = 0; ib < center.size(); ++ib) {
    BOOST_CHECK_EQUAL(xData_eq.center(ib), center[ib]);
  }

  // running into rounding errors here
  float phiStep = M_PI * 2. / 5.;
  std::vector<float> phiCenters_eq = {
      static_cast<float>(-M_PI + 0.5 * phiStep),
      static_cast<float>(-M_PI + 1.5 * phiStep),
      static_cast<float>(-M_PI + 2.5 * phiStep),
      static_cast<float>(-M_PI + 3.5 * phiStep),
      static_cast<float>(-M_PI + 4.5 * phiStep)};

  for (std::size_t ib = 0; ib < phiCenters_eq.size(); ++ib) {
    CHECK_CLOSE_ABS(phiData_eq.center(ib), phiCenters_eq[ib], 1e-3);
  }
}

// special test for phi binning
BOOST_AUTO_TEST_CASE(BinningData_phi_modules) {
  // n phi modules with phi boundary at -M_Pi/+M_PI are checked above
  // one module expands over -M_Pi/+M_PI
  float deltaPhi = 0.1;
  BinningData phiData_mod(closed, BinningValue::binPhi, 5, -M_PI + deltaPhi,
                          M_PI + deltaPhi);
  float phiStep = M_PI * 2. / 5.;
  std::vector<float> phiBoundaries_mod = {
      static_cast<float>(-M_PI + deltaPhi),
      static_cast<float>(-M_PI + 1 * phiStep) + deltaPhi,
      static_cast<float>(-M_PI + 2 * phiStep) + deltaPhi,
      static_cast<float>(-M_PI + 3 * phiStep) + deltaPhi,
      static_cast<float>(-M_PI + 4 * phiStep) + deltaPhi,
      static_cast<float>(-M_PI + 5 * phiStep) + deltaPhi};
  // this is the boundary test
  CHECK_CLOSE_REL(phiData_mod.boundaries(), phiBoundaries_mod, 1e-5);

  // now test the bin jump 0/maxbin

  float firstAngle = (-M_PI + 1.5 * deltaPhi);
  Vector3 firstBin(cos(firstAngle), sin(firstAngle), 0.);
  BOOST_CHECK_EQUAL(phiData_mod.search(firstAngle), std::size_t{0});
  BOOST_CHECK_EQUAL(phiData_mod.searchGlobal(firstBin), std::size_t{0});

  float firstAngleNeg = (-M_PI + 0.5 * deltaPhi);
  Vector3 lastBinNeg(cos(firstAngleNeg), sin(firstAngleNeg), 0.);
  BOOST_CHECK_EQUAL(phiData_mod.search(firstAngleNeg), std::size_t{4});
  BOOST_CHECK_EQUAL(phiData_mod.searchGlobal(lastBinNeg), std::size_t{4});

  float lastAnglePos = (M_PI + 0.5 * deltaPhi);
  Vector3 lastBinPos(cos(lastAnglePos), sin(lastAnglePos), 0.);
  BOOST_CHECK_EQUAL(phiData_mod.search(lastAnglePos), std::size_t{4});
  BOOST_CHECK_EQUAL(phiData_mod.searchGlobal(lastBinPos), std::size_t{4});

  // now test the (remaining) phi scaling
  float underscaledAngle = -M_PI - 0.5 * deltaPhi;
  Vector3 underscaledPos(cos(underscaledAngle), sin(underscaledAngle), 0.);
  BOOST_CHECK_EQUAL(phiData_mod.search(underscaledAngle), std::size_t{4});
  BOOST_CHECK_EQUAL(phiData_mod.searchGlobal(underscaledPos), std::size_t{4});
}

}  // namespace Acts::Test
