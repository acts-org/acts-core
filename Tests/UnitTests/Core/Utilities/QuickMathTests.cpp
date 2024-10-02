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
#include "Acts/Utilities/QuickMath.hpp"

namespace bdata = boost::unit_test::data;

const auto expDist = bdata::random(
    (bdata::engine = std::mt19937{}, bdata::seed = 0,
     bdata::distribution = std::uniform_real_distribution<double>(-4, 4)));

BOOST_AUTO_TEST_SUITE(Utilities)

BOOST_DATA_TEST_CASE(fastInverseSqrt, expDist ^ bdata::xrange(100), exp, i) {
  (void)i;

  const double x = std::pow(10, exp);

  const float fastFloat = Acts::fastInverseSqrt(static_cast<float>(x));
  const double fastDouble = Acts::fastInverseSqrt(x);

  const double stdFloat = 1.0 / std::sqrt(static_cast<float>(x));
  const double stdDouble = 1.0 / std::sqrt(x);

  CHECK_CLOSE_REL(stdFloat, fastFloat, 0.01);
  CHECK_CLOSE_REL(stdDouble, fastDouble, 0.01);
}

BOOST_DATA_TEST_CASE(fastPow, expDist ^ expDist ^ bdata::xrange(100), baseExp,
                     exp, i) {
  (void)i;

  const double base = std::pow(10, baseExp);

  const double fast = Acts::fastPow(base, exp);
  const double fastMorePrecise = Acts::fastPowMorePrecise(base, exp);

  const double std = std::pow(base, exp);

  CHECK_CLOSE_REL(fast, std, 0.15);
  CHECK_CLOSE_REL(fastMorePrecise, std, 0.1);
}

// BOOST_AUTO_TEST_CASE(fastPowChart) {
//   std::cout << "a ref obs" << std::endl;
//   for (double aExp = -4; aExp <= 4; aExp += 0.01) {
//     double a = std::pow(10, aExp);
//     double ref = std::pow(a, 0.25);
//     double obs = Acts::fastPow(a, 0.25);

//     std::cout << a << " " << ref << " " << obs << std::endl;
//   }
// }

BOOST_AUTO_TEST_SUITE_END()
