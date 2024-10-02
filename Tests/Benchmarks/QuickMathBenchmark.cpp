// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/BenchmarkTools.hpp"
#include "Acts/Utilities/QuickMath.hpp"

#include <cmath>
#include <random>

namespace bdata = boost::unit_test::data;

namespace Acts::Test {

/// @brief Another fast power function @see `fastPow`
// Taken from
// https://martin.ankerl.com/2007/02/11/optimized-exponential-functions-for-java
/// @param a the base
/// @param b the exponent
constexpr double fastPowAnother(double a, double b) {
  // enable only on IEEE 754
  static_assert(std::numeric_limits<double>::is_iec559);

  union {
    double f;
    std::int64_t i;
  } u = {};

  u.i = static_cast<std::int64_t>(
      9076650 * (a - 1) / (a + 1 + 4 * std::sqrt(a)) * b + 1072632447);
  u.i <<= 32;

  // result seems broken?
  return u.f;
}

// Some randomness & number crunching
const unsigned int nTests = 10;
const unsigned int nReps = 10000;

BOOST_DATA_TEST_CASE(
    benchmark_pow_25,
    bdata::random(
        (bdata::engine = std::mt19937(), bdata::seed = 21,
         bdata::distribution = std::uniform_real_distribution<double>(-4, 4))) ^
        bdata::xrange(nTests),
    baseExp, index) {
  (void)index;

  const double base = std::pow(10, baseExp);
  const double exp = 0.25;

  std::cout << std::endl
            << "Benchmarking base=" << base << ", exp=" << exp << "..."
            << std::endl;
  std::cout << "- void: "
            << Acts::Test::microBenchmark([&] { return 0; }, nReps)
            << std::endl;
  std::cout << "- std::pow: "
            << Acts::Test::microBenchmark([&] { return std::pow(base, exp); },
                                          nReps)
            << std::endl;
  std::cout << "- std::exp: "
            << Acts::Test::microBenchmark(
                   [&] { return std::exp(std::log(base) * exp); }, nReps)
            << std::endl;
  std::cout << "- std::sqrt: "
            << Acts::Test::microBenchmark(
                   [&] { return std::sqrt(std::sqrt(base)); }, nReps)
            << std::endl;
  std::cout << "- fastPow: "
            << Acts::Test::microBenchmark([&] { return fastPow(base, exp); },
                                          nReps)
            << std::endl;
  std::cout << "- fastPowMorePrecise: "
            << Acts::Test::microBenchmark(
                   [&] { return fastPowMorePrecise(base, exp); }, nReps)
            << std::endl;
  std::cout << "- fastPowAnother: "
            << Acts::Test::microBenchmark(
                   [&] { return fastPowAnother(base, exp); }, nReps)
            << std::endl;
}

}  // namespace Acts::Test
