// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/BenchmarkTools.hpp"

#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <cmath>
#include <complex>
#include <iostream>
#include <sstream>
#include <tuple>

namespace Acts::Test {

// Basic non-timing tests do not validate the core performance aspects of the
// benchmark tools, but have the advantage of being runnable on any system.
BOOST_AUTO_TEST_SUITE(benchmark_tools)

BOOST_AUTO_TEST_CASE(assume_accessed) {
  int x = 42;
  assumeAccessed(x);
  BOOST_CHECK_EQUAL(x, 42);
}

BOOST_AUTO_TEST_CASE(assume_read) {
  float x = 4.2f;
  assumeRead(x);
  BOOST_CHECK_EQUAL(x, 4.2f);

  const std::string y = "LOL";
  assumeRead(x);
  BOOST_CHECK_EQUAL(y, "LOL");

  assumeRead(std::make_tuple(1, false, 3.5));
}

BOOST_AUTO_TEST_CASE(assume_written) {
  std::complex c(1.2, 3.4);
  assumeWritten(c);
  BOOST_CHECK_EQUAL(c, std::complex(1.2, 3.4));
}

BOOST_AUTO_TEST_CASE(micro_benchmark_result) {
  MicroBenchmarkResult res;
  res.iters_per_run = 42;
  res.run_timings = {
      std::chrono::microseconds(420), std::chrono::microseconds(21),
      std::chrono::milliseconds(4),   std::chrono::microseconds(84),
      std::chrono::microseconds(294), std::chrono::microseconds(378),
      std::chrono::microseconds(126), std::chrono::milliseconds(42)};

  CHECK_CLOSE_REL(res.totalTime().count() / 1'000'000., 47.323, 1e-6);

  const auto sorted = res.sortedRunTimes();
  BOOST_CHECK_EQUAL(sorted.size(), res.run_timings.size());
  BOOST_CHECK_EQUAL(sorted[0].count(), 21'000.);
  BOOST_CHECK_EQUAL(sorted[1].count(), 84'000.);
  BOOST_CHECK_EQUAL(sorted[2].count(), 126'000.);
  BOOST_CHECK_EQUAL(sorted[3].count(), 294'000.);
  BOOST_CHECK_EQUAL(sorted[4].count(), 378'000.);
  BOOST_CHECK_EQUAL(sorted[5].count(), 420'000.);
  BOOST_CHECK_EQUAL(sorted[6].count(), 4'000'000.);
  BOOST_CHECK_EQUAL(sorted[7].count(), 42'000'000.);

  CHECK_CLOSE_REL(res.runTimeMedian().count() / 1000., (294. + 378.) / 2.,
                  1e-6);

  const auto [firstq, thirdq] = res.runTimeQuartiles();
  CHECK_CLOSE_REL(firstq.count() / 1000., (84. + 126.) / 2., 1e-6);
  CHECK_CLOSE_REL(thirdq.count() / 1000., (420. + 4000.) / 2., 1e-6);

  const auto robustRTStddev = res.runTimeRobustStddev();
  CHECK_CLOSE_REL(robustRTStddev.count(), (thirdq - firstq).count() / 1.349,
                  1e-3);

  const auto runTimeError = res.runTimeError();
  CHECK_CLOSE_REL(
      runTimeError.count(),
      1.2533 * robustRTStddev.count() / std::sqrt(res.run_timings.size()),
      1e-3);

  CHECK_CLOSE_REL(res.iterTimeAverage().count(),
                  res.runTimeMedian().count() / res.iters_per_run, 1e-6);

  CHECK_CLOSE_REL(res.iterTimeError().count(),
                  runTimeError.count() / std::sqrt(res.iters_per_run), 1e-6);

  std::ostringstream os;
  os << res;
  BOOST_CHECK_EQUAL(os.str(),
                    "8 runs of 42 iteration(s), 47.3ms total, "
                    "336.0000+/-1355.2296µs per run, "
                    "8000.000+/-209116.462ns per iteration");
}

BOOST_AUTO_TEST_CASE(micro_benchmark) {
  int counter = 0;
  microBenchmark([&] { ++counter; }, 15, 7, std::chrono::milliseconds(0));
  BOOST_CHECK_EQUAL(counter, 15 * 7);

  counter = 0;
  microBenchmark(
      [&] {
        ++counter;
        return counter;
      },
      17, 11, std::chrono::milliseconds(0));
  BOOST_CHECK_EQUAL(counter, 17 * 11);

  counter = 0;
  int previous = 64;
  std::vector<int> ints{1, 2, 4, 8, 16, 32, 64};
  microBenchmark(
      [&](int input) {
        if (input == 1) {
          BOOST_CHECK_EQUAL(previous, 64);
          counter = 1;
        } else {
          BOOST_CHECK_EQUAL(input, previous * 2);
          counter += input;
        }
        previous = input;
      },
      ints, 123, std::chrono::milliseconds(3));
  BOOST_CHECK_EQUAL(counter, 127);

  counter = 0;
  previous = -81;
  std::vector<char> chars{-1, 3, -9, 27, -81};
  microBenchmark(
      [&](int input) {
        if (input == -1) {
          BOOST_CHECK_EQUAL(previous, -81);
          counter = -1;
        } else {
          BOOST_CHECK_EQUAL(input, -previous * 3);
          counter += input;
        }
        previous = input;
        return &previous;
      },
      chars, 456, std::chrono::milliseconds(8));
  BOOST_CHECK_EQUAL(counter, -61);
}

BOOST_AUTO_TEST_SUITE_END()

// Timing tests are perhaps the most important ones for validation of
// benchmarking tools, but they cannot be run by default for two reasons:
// - They take a while to run, and therefore slow down the testing cycle
// - They require a quiet system to succeed, and will likely fail when invoked
//   by a parallel run of CTest or when run on a continuous integration VM.
//
// If you can ensure both of these preconditions, you can run the test with
// ./BenchmarkTools --run_test=benchmark_timings
BOOST_AUTO_TEST_SUITE(benchmark_timings, *boost::unit_test::disabled())

constexpr std::size_t bench_iters = 1'000;

BOOST_AUTO_TEST_CASE(micro_benchmark) {
  using namespace std::literals::chrono_literals;

  // For simple microbenchmarking needs, plain use of microBenchmark is enough.
  //
  // For example, here, the microbenchmark loop isn't optimized out even though
  // each iteration does literally nothing. If it were optimized out, the time
  // per iteration would change, since we wouldn't get linear scaling anymore.
  auto nop = [] {};
  const auto nop_x10 = microBenchmark(nop, 10 * bench_iters);
  std::cout << "nop (10x iters): " << nop_x10 << std::endl;
  const auto nop_x100 = microBenchmark(nop, 100 * bench_iters);
  std::cout << "nop (100x iters): " << nop_x100 << std::endl;
  const double nop_x10_iter_ns = nop_x10.iterTimeAverage().count();
  const double nop_x100_iter_ns = nop_x100.iterTimeAverage().count();
  CHECK_CLOSE_REL(nop_x10_iter_ns, nop_x100_iter_ns, 0.1);

// These tests reason about the performance characteristics of _optimized_ code,
// and should therefore be compiled out of debug/coverage builds.
#ifdef __OPTIMIZE__
  // The microbenchmarking harness is super low overhead, less than 1
  // nanosecond per iteration on a modern CPU.
  BOOST_CHECK_LT(nop_x100_iter_ns, 1.0);

  // With a well-chosen iteration count that keeps per-run times under the OS
  // scheduling quantum (typically 1ms), the noise is also super low.
  BOOST_CHECK_LT(nop_x100.iterTimeError().count(), 0.1);

  // You can measure the overhead of any operation as long as it's not
  // _obnoxiously_ amenable to compiler const-propagation or dead code
  // elimination. For example, this sqrt throughput microbenchmark works,
  // because microBenchmark forces the compiler to assume that "x", "y" and "z"
  // are modified on every benchmark iteration...
  const double x = 1.2, y = 3.4, z = 5.6;
  auto sqrt = microBenchmark(
      [&] { return std::sqrt(x * y) + std::sqrt(y * z) + std::sqrt(z * x); },
      bench_iters);
  std::cout << "sqrt (correct): " << sqrt << std::endl;
  BOOST_CHECK_GT(sqrt.iterTimeAverage().count(), 10. * nop_x100_iter_ns);

  // ...but this variant doesn't work, because the compiler can trivially
  // precompute the square root when optimizing the inner lambda...
  const auto sqrt_constprop = microBenchmark(
      [] {
        return std::sqrt(1.2 * 3.4) + std::sqrt(3.4 * 5.6) +
               std::sqrt(5.6 * 1.2);
      },
      bench_iters * 20);
  std::cout << "sqrt (constprop'd): " << sqrt_constprop << std::endl;
  BOOST_CHECK_LT(sqrt_constprop.iterTimeAverage().count(),
                 sqrt.iterTimeAverage().count() / 5.);

  // ...and this one doesn't work either, because the compiler can trivially
  // infer that the result of the computation is unused and stop computing it.
  //
  // The lower tolerance of this test is needed because current GCC doesn't
  // optimize _everything_ out in its default configuration, as sqrt could still
  // have side-effects like setting the errno thread-local variable...
  const auto sqrt_deadcode = microBenchmark(
      [&] { (void)(std::sqrt(x * y) + std::sqrt(y * z) + std::sqrt(z * x)); },
      bench_iters * 10);
  std::cout << "sqrt (deadcode'd): " << sqrt_deadcode << std::endl;
  BOOST_CHECK_LT(sqrt_deadcode.iterTimeAverage().count(),
                 sqrt.iterTimeAverage().count() / 3.);
#endif
}

// These tests reason about the performance characteristics of _optimized_ code,
// and should therefore be compiled out of debug/coverage builds.
#ifdef __OPTIMIZE__
BOOST_AUTO_TEST_CASE(assume_read) {
  // You can use assumeRead when you want the compiler to assume that the result
  // of some computation has been read and therefore the computation shouldn't
  // be optimized out. This is what microBenchmark implicitly does to the value
  // returned by the benchmark iteration function, if any.
  //
  // For example, these two computations are almost equivalent. Notice that
  // assumeRead can be used on temporaries.
  const double x = 1.2, y = 3.4, z = 5.6;
  const auto tuple_return = microBenchmark(
      [&] {
        return std::make_tuple(
            std::sqrt(x * y), std::complex(std::sqrt(y * z), std::sqrt(z * x)));
      },
      bench_iters);
  std::cout << "tuple return: " << tuple_return << std::endl;
  const auto assumeread = microBenchmark(
      [&] {
        assumeRead(std::sqrt(x * y));
        assumeRead(std::complex(std::sqrt(y * z), std::sqrt(z * x)));
      },
      bench_iters);
  std::cout << "assumeRead: " << assumeread << std::endl;
  const double tuple_return_iter_ns = tuple_return.iterTimeAverage().count();
  const double assumeRead_iter_ns = assumeread.iterTimeAverage().count();
  CHECK_CLOSE_REL(tuple_return_iter_ns, assumeRead_iter_ns, 1e-2);
}
#endif

BOOST_AUTO_TEST_CASE(assume_written) {
  // You can use assumeWritten when you want the compiler to assume that some
  // variables have been written to, and every dependent computation must
  // therefore be recomputed. This is what microBenchmark implicitly does to
  // every variable captured by the benchmark iteration lambda.
  //
  // Since assumeWritten operates on variables in memory, it cannot be used on
  // temporaries, but only on mutable variables.
  double x = 1.2, y = 3.4, z = 5.6;
  auto sqrt_sum = microBenchmark(
      [&] { return std::sqrt(x * y) + std::sqrt(y * z) + std::sqrt(z * x); },
      bench_iters);
  std::cout << "sqrt sum: " << sqrt_sum << std::endl;
  auto sqrt_2sums = microBenchmark(
      [&] {
        double tmp = std::sqrt(x * y) + std::sqrt(y * z) + std::sqrt(z * x);
        assumeWritten(x);
        assumeWritten(y);
        assumeWritten(z);
        return tmp + std::sqrt(x * y) + std::sqrt(y * z) + std::sqrt(z * x);
      },
      bench_iters);
  std::cout << "2x(sqrt sum): " << sqrt_2sums << std::endl;
  const double sqrt_sum_iter_ns = sqrt_sum.iterTimeAverage().count();
  const double sqrt_2sums_iter_ns = sqrt_2sums.iterTimeAverage().count();
  CHECK_CLOSE_REL(2. * sqrt_sum_iter_ns, sqrt_2sums_iter_ns, 1e-2);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
