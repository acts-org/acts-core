// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/RangeXD.hpp"

#include <limits>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Utilities)

BOOST_AUTO_TEST_SUITE(RangeXD)

BOOST_AUTO_TEST_CASE(default_constructor_double) {
  Acts::RangeXD<3, double> r;

  BOOST_CHECK_EQUAL(r[0].min(), std::numeric_limits<double>::lowest());
  BOOST_CHECK_EQUAL(r[0].max(), std::numeric_limits<double>::max());
  BOOST_CHECK_EQUAL(r[1].min(), std::numeric_limits<double>::lowest());
  BOOST_CHECK_EQUAL(r[1].max(), std::numeric_limits<double>::max());
  BOOST_CHECK_EQUAL(r[2].min(), std::numeric_limits<double>::lowest());
  BOOST_CHECK_EQUAL(r[2].max(), std::numeric_limits<double>::max());
}

BOOST_AUTO_TEST_CASE(mutate_ranges_double) {
  Acts::RangeXD<3, double> r;

  r[0].shrink(-10.0, 10.0);
  r[1].shrink(0.0, 50.0);
  r[2].shrinkMax(-20.0);

  BOOST_CHECK_EQUAL(r[0].min(), -10.0);
  BOOST_CHECK_EQUAL(r[0].max(), 10.0);
  BOOST_CHECK_EQUAL(r[1].min(), 0.0);
  BOOST_CHECK_EQUAL(r[1].max(), 50.0);
  BOOST_CHECK_EQUAL(r[2].min(), std::numeric_limits<double>::lowest());
  BOOST_CHECK_EQUAL(r[2].max(), -20.0);
}

BOOST_AUTO_TEST_CASE(degenerate_double) {
  Acts::RangeXD<3, double> r;

  r[0].shrink(-10.0, 10.0);
  r[1].shrink(0.0, 50.0);
  r[2].shrink(-20.0, -30.0);

  BOOST_CHECK(r.degenerate());
}

BOOST_AUTO_TEST_CASE(degenerate_false_double) {
  Acts::RangeXD<3, double> r;

  r[0].shrink(-10.0, 10.0);
  r[1].shrink(0.0, 50.0);
  r[2].shrink(-20.0, 30.0);

  BOOST_CHECK(!r.degenerate());
}

BOOST_AUTO_TEST_CASE(contains_double) {
  Acts::RangeXD<3, double> r;

  r[0].shrink(-10.0, 10.0);
  r[1].shrink(0.0, 50.0);
  r[2].shrink(-20.0, 30.0);

  BOOST_CHECK(r.contains({0.0, 1.0, 0.0}));
  BOOST_CHECK(r.contains({9.0, 0.5, 29.0}));
  BOOST_CHECK(r.contains({-8.0, 40.0, -20.0}));
  BOOST_CHECK(r.contains({4.0, 4.0, 4.0}));
  BOOST_CHECK(r.contains({0.0, 0.0, 0.0}));
  BOOST_CHECK(!r.contains({-12.0, 1.0, 0.0}));
  BOOST_CHECK(!r.contains({0.0, -1.0, 0.0}));
  BOOST_CHECK(!r.contains({0.0, 1.0, 40.0}));
  BOOST_CHECK(!r.contains({20.0, 10.0, 300.0}));
  BOOST_CHECK(!r.contains({-100.0, -100.0, -100.0}));
}

BOOST_AUTO_TEST_CASE(equality_int) {
  Acts::RangeXD<3, int> r, q;

  r[0].shrink(-10, 10);
  r[1].shrink(0, 50);
  r[2].shrink(-20, 30);

  q[0].shrink(-10, 10);
  q[1].shrink(0, 50);
  q[2].shrink(-20, 30);

  BOOST_CHECK((r == q));
}

BOOST_AUTO_TEST_CASE(subset1_double) {
  Acts::RangeXD<3, double> r, q;

  r[0].shrink(-10.0, 10.0);
  r[1].shrink(0.0, 50.0);
  r[2].shrink(-20.0, 30.0);

  q[0].shrink(-5.0, 5.0);
  q[1].shrink(10.0, 100.0);
  q[2].shrink(-10.0, 20.0);

  BOOST_CHECK(!(r <= q));
  BOOST_CHECK(!(q <= r));
}

BOOST_AUTO_TEST_CASE(subset2_double) {
  Acts::RangeXD<3, double> r, q;

  r[0].shrink(-10.0, 10.0);
  r[1].shrink(0.0, 50.0);
  r[2].shrink(-20.0, 30.0);

  q[0].shrink(-5.0, 5.0);
  q[1].shrink(10.0, 20.0);
  q[2].shrink(-10.0, 20.0);

  BOOST_CHECK(!(r <= q));
  BOOST_CHECK((q <= r));
}

BOOST_AUTO_TEST_CASE(superset2_double) {
  Acts::RangeXD<3, double> r, q;

  r[0].shrink(-10.0, 10.0);
  r[1].shrink(0.0, 50.0);
  r[2].shrink(-20.0, 30.0);

  q[0].shrink(-5.0, 5.0);
  q[1].shrink(10.0, 20.0);
  q[2].shrink(-10.0, 20.0);

  BOOST_CHECK((r >= q));
  BOOST_CHECK(!(q >= r));
}

BOOST_AUTO_TEST_CASE(intersection_double) {
  Acts::RangeXD<3, double> r, q;

  r[0].shrink(-10.0, 10.0);
  r[1].shrink(0.0, 50.0);
  r[2].shrink(-20.0, 30.0);

  q[0].shrink(-5.0, 5.0);
  q[1].shrink(10.0, 20.0);
  q[2].shrink(-10.0, 20.0);

  BOOST_CHECK((r && q));
  BOOST_CHECK((q && r));
}

BOOST_AUTO_TEST_CASE(intersection_false_double) {
  Acts::RangeXD<3, double> r, q;

  r[0].shrink(-10.0, 10.0);
  r[1].shrink(0.0, 50.0);
  r[2].shrink(-20.0, 30.0);

  q[0].shrink(-5.0, 5.0);
  q[1].shrink(10.0, 20.0);
  q[2].shrink(-100.0, -50.0);

  BOOST_CHECK(!(r && q));
  BOOST_CHECK(!(q && r));
}

BOOST_AUTO_TEST_CASE(intersection_false2_double) {
  Acts::RangeXD<3, double> r, q;

  r[0].shrink(-10.0, 10.0);
  r[1].shrink(0.0, 50.0);
  r[2].shrink(-20.0, 30.0);

  q[0].shrink(-50.0, -20.0);
  q[1].shrink(-10.0, -5.0);
  q[2].shrink(-100.0, -50.0);

  BOOST_CHECK(!(r && q));
  BOOST_CHECK(!(q && r));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
