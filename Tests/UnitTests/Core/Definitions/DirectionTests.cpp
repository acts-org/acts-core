// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"

#include <string>

using namespace Acts;

BOOST_AUTO_TEST_SUITE(DefinitionsDirection)

BOOST_AUTO_TEST_CASE(DirectionTests) {
  constexpr Direction bwd = Direction::Backward;
  constexpr Direction fwd = Direction::Forward;

  BOOST_CHECK_EQUAL(bwd, Direction::Negative);
  BOOST_CHECK_EQUAL(fwd, Direction::Positive);

  BOOST_CHECK_EQUAL(Direction::fromScalar(-1.), bwd);
  BOOST_CHECK_EQUAL(Direction::fromScalar(1.), fwd);
  BOOST_CHECK_EQUAL(Direction::fromScalarZeroAsPositive(0), fwd);

  BOOST_CHECK_EQUAL(Direction::fromIndex(0), bwd);
  BOOST_CHECK_EQUAL(Direction::fromIndex(1), fwd);

  BOOST_CHECK_EQUAL(bwd.index(), 0u);
  BOOST_CHECK_EQUAL(fwd.index(), 1u);

  BOOST_CHECK_EQUAL(bwd.sign(), -1);
  BOOST_CHECK_EQUAL(fwd.sign(), +1);

  BOOST_CHECK_EQUAL(bwd.invert(), fwd);
  BOOST_CHECK_EQUAL(fwd.invert(), bwd);

  BOOST_CHECK_EQUAL(bwd.toString(), "backward");
  BOOST_CHECK_EQUAL(fwd.toString(), "forward");

  BOOST_CHECK_EQUAL(2. * fwd, 2.);
  BOOST_CHECK_EQUAL(7 * fwd, 7);
  BOOST_CHECK_EQUAL(Vector3(1., 1., 1.) * fwd, Vector3(1., 1., 1.));

  BOOST_CHECK_EQUAL(2. * bwd, -2.);
  BOOST_CHECK_EQUAL(7 * bwd, -7);
  BOOST_CHECK_EQUAL(Vector3(1., 1., 1.) * bwd, Vector3(-1., -1., -1.));

  double a = 7.;
  a *= fwd;
  BOOST_CHECK_EQUAL(a, 7.);
  a *= bwd;
  BOOST_CHECK_EQUAL(a, -7.);

  float b = 8.;
  b *= fwd;
  BOOST_CHECK_EQUAL(b, 8.);
  b *= bwd;
  BOOST_CHECK_EQUAL(b, -8.);

  Vector3 c(9., 9., 9.);
  c *= fwd;
  BOOST_CHECK_EQUAL(c, Vector3(9., 9., 9.));
  c *= bwd;
  BOOST_CHECK_EQUAL(c, Vector3(-9., -9., -9.));
}

BOOST_AUTO_TEST_SUITE_END()
