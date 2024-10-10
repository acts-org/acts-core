// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/MathHelpers.hpp"

#include <cmath>

namespace bdata = boost::unit_test::data;

const auto expDist = bdata::random(
    (bdata::engine = std::mt19937{}, bdata::seed = 0,
     bdata::distribution = std::uniform_real_distribution<double>(-4, 4)));

BOOST_AUTO_TEST_SUITE(Utilities)

BOOST_DATA_TEST_CASE(fastHypot, expDist ^ expDist ^ bdata::xrange(100), xExp,
                     yExp, i) {
  (void)i;

  const double x = std::pow(10, xExp);
  const double y = std::pow(10, yExp);

  const float fastFloat =
      Acts::fastHypot(static_cast<float>(x), static_cast<float>(y));
  const double fastDouble = Acts::fastHypot(x, y);

  const float stdFloat =
      std::hypot(static_cast<float>(x), static_cast<float>(y));
  const double stdDouble = std::hypot(x, y);

  CHECK_CLOSE_REL(stdFloat, fastFloat, 1e-6);
  CHECK_CLOSE_REL(stdDouble, fastDouble, 1e-6);
}

BOOST_AUTO_TEST_SUITE_END()
