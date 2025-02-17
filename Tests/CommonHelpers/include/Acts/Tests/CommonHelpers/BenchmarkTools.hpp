// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <numbers>
#include <numeric>
#include <ostream>
#include <type_traits>
#include <utility>
#include <vector>

namespace Acts::Test {

// === INTRODUCTION ===
//
// This header contains tools which can be used to avoid unrealistic
// compiler optimizations in microbenchmarks, such as elimination of unused
// benchmark computations or deduplication of benchmark loops which repeat
// the same operation N times with unchanging inputs.
//
// ### WARNING ON HARDWARE OPTIMIZATIONS ###
//
// When using these tools, you must keep in mind that compiler optimizations
// are only a part of the story, and CPUs will also perform a bunch of dynamic
// optimizations on their own, including but by no means limited to...
//
// - Moving small benchmark inputs into fast caches
// - Buffering and caching small benchmark outputs
// - Instruction-level parallelization (pipelining, superscalar
//   execution...) of supposedly unrelated benchmark iterations
// - "Turbo" overclocking of CPU cores which is only sustainable in specific
//   circumstances (short-running code sections, single-core execution...)
// - Branch predictor training for specific loops and conditionals
//
// Unfortunately, there is no way to disable those hardware optimizations in
// a way which meets even basic sanity requirements:
//
// - Easily portable to all CPU models of interest
// - Fine-grained enough to target CPU optimizations which are an
//   unrealistic artifact of the microbenchmark, without also disabling
//   optimizations which will happen in realistic usage scenarios (like
//   hot code remaining resident in the CPU instruction cache)
// - Generic enough for all benchmark inputs, outputs, and code
// - Minimal impact on overall benchmark timings
//
// You will thus need to live with the fact that any microbenchmark has
// hardware bias, which is not as egregious as compiler bias (since the CPU
// can't just delete gigantic sections of the code like the compiler does)
// but still very much significant on simple benchmarks.
//
// The proper way to detect and correct this bias is not to try to write more
// clever microbenchmarks. It is to use realistic benchmarks of full app
// workloads whenever possible, and treat microbenchmarks only as a tool for
// detecting performance regressions in known-critical components and
// easily fine-tuning the performance of these components, in a manner whose
// effectiveness will later have to be checked on a more realistic benchmark.
//
// #########################################
//
//
// === OPTIMIZATION BARRIERS ===
//
// Mark some data as read and written in the eye of the compiler, so it can't
// optimize under the assumption that the data isn't used or does not change.
//
// The current implementation has limitations that you should bear in mind:
//
// - "clobber" data which resides in CPU registers must be flushed to memory
//   and reloaded from memory afterwards if re-used. This will increase memory
//   traffic and cache footprint in a potentially unrealistic fashion.
// - Putting this optimization barrier on every iteration of a loop will
//   prevent compiler loop optimizations like autovectorization, which is
//   generally too strong. Consider putting the barrier every N loop
//   iterations instead (and checking for various N), or at the very end of
//   the loop/benchmark if the memory traffic of storing all the end results
//   doesn't bias your benchmark too much.
// - The barrier's implementation uses a very basic subset of the GCC dialect of
//   inline assembly, which although widely supported (from a quick godbolt
//   check, in addition to GCC it works on clang, djgpp, ellcc, icc and zapcc),
//   is most notably not supported by MSVC. I have not yet found an MSVC or
//   portable equivalent which is UB-free and works even when passing in a
//   pointer to a local variable, suggestions and patches are welcome.
//
#ifdef __GNUC__

template <typename T>
inline void assumeAccessed(T&& clobber) {
  // This optimization barrier leverages the fact that inline ASM isn't smart,
  // and couldn't get smarter as the compiler would then need to be able to
  // parse and understand the side effects of arbitrary assembly statements,
  // which would be a ridiculous amount of work for compiler devs:
  //
  // - The compiler only performs trivial analysis of the assembly (e.g.
  //   number of instructions for inlining purposes), so it can't leverage the
  //   fact that it's empty to optimize out its inputs. It must assume that
  //   the assembly statements will use the declared inputs, emit the declared
  //   outputs, and read/write the contents of declared clobbers.
  // - A pointer to "clobber" is declared as an input to assembly, and the
  //   "memory" clobber allows the assembly statement to read and write memory
  //   targeted by this pointer. Therefore, the compiler must compute and
  //   write down the value of "clobber" in memory, and reload it into
  //   registers if it's used later on by the program.
  // - A volatile marker is used, therefore the compiler can't optimize out
  //   the asm statement even though from its specification it should have no
  //   program-visible side effects. It also prevents the compiler from moving
  //   the asm statement out of a loop, which would be bad... But note that it
  //   does _not_ prevent it from moving within a loop iteration.
  //
  __asm__ volatile("" : : "g"(&clobber) : "memory");
}

#else

template <typename T>
void assumeAccessed(T&& clobber) {
  // FIXME: Find a reliable optimization barrier for MSVC. As a litmus test, the
  //        assembly generated for the following code should store "42" in
  //        memory twice, not once:
  //
  //        ```
  //        int x = 42;
  //        assumeAccessed(x);
  //        x = 42;
  //        assumeAccessed(&x);
  //        ```
  //
  static_assert(false, "No optimization barrier available for this compiler");
}

#endif

// Mark some data as read in the eye of the compiler, so it can't optimize out
// the source computation under the assumption that its output isn't used.
//
// Has the same caveats as assumeAccessed().
//
template <typename T>
inline void assumeRead(const T& clobber) {
  // FIXME: I don't know of a finer-grained compiler optimization barrier that
  //        1/can be used when one only wants to fake a read and 2/works for
  //        all inputs (not just machine types), so for now this function is
  //        only a way to clarify developer intent.
  assumeAccessed(clobber);
}

// Mark some data as written in the eye of the compiler, so it can't optimize
// out repetitive dependent computations under the assumption that the result
// will always be the same (and more generally can't assume anything about the
// value of this input beyond type-level properties).
//
// Has the same caveats as assumeAccessed().
//
template <typename T>
inline void assumeWritten(T& clobber) {
  // FIXME: I don't know of a finer-grained compiler optimization barrier that
  //        1/can be used when one only wants to fake a write and 2/ works for
  //        all outputs (not just machine types), so for now this
  //        function is only a way to clarify developer intent.
  assumeAccessed(clobber);
}
//
//
// === MICROBENCHMARK HARNESS ===

// Results of a microbenchmark
//
// Holds the timings of each benchmark run, and allows the user to query various
// statistical information about them.
//
struct MicroBenchmarkResult {
  using Duration = std::chrono::duration<double, std::nano>;

  std::size_t iters_per_run = 0;
  std::vector<Duration> run_timings;

  // Total benchmark running time
  //
  // Unless your machine has been tuned for extremely stable timings (minimal
  // background activity, no CPU frequency scaling, no disk spin-down...), you
  // shouldn't give much credence to benchmarks with running times smaller than
  // a few hundreds of milliseconds.
  //
  Duration totalTime() const {
    return std::accumulate(run_timings.cbegin(), run_timings.cend(),
                           Duration());
  }

  // Robust estimator of the mean benchmark iteration time
  //
  // Computed as the average iteration time of the median benchmark run, this
  // estimator provides a tunable compromise between the mean and median
  // estimators of the benchmark iteration time:
  //
  // - As a mean iteration time, it can be measured with low overhead by tuning
  //   iters_per_run up until the impact of time measurement on benchmark
  //   timings is negligible. It also converges with optimal efficiency on
  //   unbiased data as iters_per_run increases.
  // - Being based on the median run timing, this estimator is also robust
  //   against outlier measurements, such as timing spikes originating from
  //   bursts of background system load. The more benchmark runs you measure,
  //   the higher the robustness.
  //
  // This analysis assumes that the run timing distribution is roughly
  // symmetric (and therefore the median can be used as an estimator of the
  // mean), but makes no assumption about iteration time distributions.
  //
  Duration iterTimeAverage() const {
    assert(iters_per_run > 0);
    return runTimeMedian() / iters_per_run;
  }

  // Standard error on the estimator of the mean benchmark iteration time
  //
  // This analysis assumes that the run timing distribution is normal because
  // the underlying `runTimeError` analysis does. It also assumes that
  // per-iteration times are independent and identically distributed.
  //
  Duration iterTimeError() const {
    assert(iters_per_run > 0);
    return runTimeError() / std::sqrt(iters_per_run);
  }

  // Sorted benchmark run times, used for computing outlier-robust statistics
  std::vector<Duration> sortedRunTimes() const {
    std::vector<Duration> sorted_timings = run_timings;
    std::ranges::sort(sorted_timings);
    return sorted_timings;
  }

  // Median time per benchmark run
  //
  // This is an outlier-robust estimator of the mean benchmark run time if the
  // run time distribution is roughly symmetric.
  //
  Duration runTimeMedian() const {
    assert(!run_timings.empty());
    const std::vector<Duration> sorted_timings = sortedRunTimes();
    const std::size_t midpoint = sorted_timings.size() / 2;
    if (sorted_timings.size() % 2 == 0) {
      return (sorted_timings[midpoint - 1] + sorted_timings[midpoint]) / 2;
    } else {
      return sorted_timings[midpoint];
    }
  }

  // First and third quartiles of benchmark run time timings
  std::pair<Duration, Duration> runTimeQuartiles() const {
    // Unfortunately, quartile computations on datasets whose size is not a
    // multiple of 4 are not standardized. We use an interpolation- and
    // symmetry-based definition that follows all consensual properties:
    //
    // - When the dataset size is a multiple of 4, the first quantile is
    //   the mean of the last point of the first quarter of the sorted dataset
    //   (called first_point below) and the first point of the second quarter of
    //   the dataset (which is the next point). This is universally agreed upon.
    // - More generally, when the dataset size is a multiple of 2, the first
    //   quantile is the median of the first half of the sorted dataset. Most
    //   commonly used quantile definitions agree on this, but some don't.
    // - The third quantile is defined symmetrically with respect to the first
    //   one, starting from the end of the sorted dataset and going downwards.
    //
    assert(run_timings.size() >= 2);
    const std::vector<Duration> sorted_timings = sortedRunTimes();
    const std::size_t first_point = (sorted_timings.size() - 2) / 4;
    const std::size_t offset = (sorted_timings.size() - 2) % 4;
    const std::size_t third_point = (sorted_timings.size() - 1) - first_point;
    if (offset == 0) {
      return {sorted_timings[first_point], sorted_timings[third_point]};
    } else {
      const auto first_quartile = ((4 - offset) * sorted_timings[first_point] +
                                   offset * sorted_timings[first_point + 1]) /
                                  4;
      const auto third_quartile = ((4 - offset) * sorted_timings[third_point] +
                                   offset * sorted_timings[third_point - 1]) /
                                  4;
      return {first_quartile, third_quartile};
    }
  }

  // Robust estimator of benchmark run timing standard deviation
  //
  // Assuming that the run time distribution is normal aside from occasional
  // outlier pollution, an outlier-robust, unbiased, and consistent estimator of
  // its standard deviation can be built from the interquartile range via
  // the formula estimated_stddev = IQR / (2 * sqrt(2) * erf-1(1/2)).
  //
  // This analysis assumes that the run timing distribution is roughly normal,
  // outlier measurements aside.
  //
  Duration runTimeRobustStddev() const {
    auto [firstq, thirdq] = runTimeQuartiles();
    return (thirdq - firstq) /
           (2. * std::numbers::sqrt2 * 0.4769362762044698733814);
  }

  // Standard error on the median benchmark run time
  //
  // This analysis assumes that the run timing distribution is approximately
  // normal, a few outliers aside.
  //
  Duration runTimeError() const {
    return 1.2533 * runTimeRobustStddev() / std::sqrt(run_timings.size());
  }

  // Standardized display for benchmark statistics
  //
  // The underlying data analysis assumes the full mathematical contract stated
  // in the documentations of `runTimeError`, `iterTimeAverage` and
  // `iterTimeError`. It also assumes that _both_ the run and iteration timings
  // follow an approximately normal distribution, so that the standard 95%
  // confidence interval formulation can be applied and that the median run
  // time can be used as an estimator of the mean run time.
  //
  friend std::ostream& operator<<(std::ostream& os,
                                  const MicroBenchmarkResult& res) {
    auto old_precision = os.precision();
    auto old_flags = os.flags();
    os << std::fixed << res.run_timings.size() << " runs of "
       << res.iters_per_run << " iteration(s), " << std::setprecision(1)
       << res.totalTime().count() / 1'000'000 << "ms total, "
       << std::setprecision(4) << res.runTimeMedian().count() / 1'000 << "+/-"
       << 1.96 * res.runTimeError().count() / 1'000 << "µs per run, "
       << std::setprecision(3) << res.iterTimeAverage().count() << "+/-"
       << 1.96 * res.iterTimeError().count() << "ns per iteration";
    os.precision(old_precision);
    os.flags(old_flags);
    return os;
  }
};

// Implementation details, scroll down for more public API
namespace benchmark_tools_internal {

// General iteration of microBenchmark with inputs and outputs
template <typename Callable, typename Input, typename Result>
struct MicroBenchmarkIterImpl {
  static inline void iter(const Callable& iteration, const Input& input) {
    assumeWritten(iteration);
    assumeWritten(input);
    const auto result = iteration(input);
    assumeRead(result);
  }
};

// Specialization for void(Input) functors, where there is no output
template <typename Callable, typename Input>
struct MicroBenchmarkIterImpl<Callable, Input, void> {
  static inline void iter(const Callable& iteration, const Input& input) {
    assumeWritten(iteration);
    assumeWritten(input);
    iteration(input);
  }
};

// Specialization for Result(void) functors, where there is no input
template <typename Callable, typename Result>
struct MicroBenchmarkIterImpl<Callable, void, Result> {
  static inline void iter(const Callable& iteration) {
    assumeWritten(iteration);
    const auto result = iteration();
    assumeRead(result);
  }
};

// Specialization for void() functors, where there is no input and no output
template <typename Callable>
struct MicroBenchmarkIterImpl<Callable, void, void> {
  static inline void iter(const Callable& iteration) {
    assumeWritten(iteration);
    iteration();
  }
};

template <typename T, typename I>
using call_with_input_t = decltype(std::declval<T>()(std::declval<I>()));

template <typename T>
using call_without_input_t = decltype(std::declval<T>()());

// If callable is a callable that takes the expected input argument type, then
// this specialization will be selected...
template <typename Callable, typename Input = void>
struct MicroBenchmarkIter {
  static inline void iter(const Callable& iteration, const Input* input)
    requires std::invocable<Callable, Input>
  {
    using Result = std::invoke_result_t<Callable, const Input&>;
    MicroBenchmarkIterImpl<Callable, Input, Result>::iter(iteration, *input);
  }
};

// If Callable is a callable that takes no argument, this specialization will be
// picked instead of the one above...
template <typename Callable>
struct MicroBenchmarkIter<Callable, void> {
  static inline void iter(const Callable& iteration,
                          const void* /*input*/ = nullptr)
    requires std::invocable<Callable>
  {
    using Result = std::invoke_result_t<Callable>;
    MicroBenchmarkIterImpl<Callable, void, Result>::iter(iteration);
  }
};

// Common logic between iteration-based and data-based microBenchmark
template <typename Callable>
MicroBenchmarkResult microBenchmarkImpl(Callable&& run,
                                        std::size_t iters_per_run,
                                        std::size_t num_runs,
                                        std::chrono::milliseconds warmup_time) {
  using Clock = std::chrono::steady_clock;

  MicroBenchmarkResult result;
  result.iters_per_run = iters_per_run;
  result.run_timings = std::vector(num_runs, MicroBenchmarkResult::Duration());

  const auto warmup_start = Clock::now();
  while (Clock::now() - warmup_start < warmup_time) {
    run();
  }

  for (std::size_t i = 0; i < num_runs; ++i) {
    const auto start = Clock::now();
    run();
    result.run_timings[i] = Clock::now() - start;
  }

  return result;
}

}  // namespace benchmark_tools_internal

// Run a user-provided benchmark `iteration` function that takes no argument
// in batches of `iters_per_run` iterations, for `num_runs` batches, after some
// warmup period of `warmup_time` has elapsed. Return execution statistics.
//
// The output of `iteration` is marked as read, so the compiler cannot optimize
// it out except by const-propagation (i.e. it is a function of a constexpr
// quantity). If `iteration` is a lambda which captures inputs from a higher
// scope, those are marked as written on every iteration as well, so the
// compiler cannot optimize out the iteration loop into a single iteration.
//
// Together, these precautions void the need for manually calling assumeRead and
// assumeWritten in all simple cases where the benchmark iteration function
// ingests some inputs from an outer scope and emits the output of its
// computations as a return value.
//
// We do batched runs instead of using a single iteration loop because:
// - It allows us to provide error bars on the timing measurement, which allows
//   comparing two measurements and detecting timing jitter problems emerging
//   from e.g. excessive background system activity.
// - For short-running iteration functions, it allows you to keep benchmark runs
//   much shorter than one OS scheduling quantum (typically ~1ms), which enables
//   high-precision measurements devoid of any scheduling-induced timing jitter,
//   while still aggregating as much statistics as you want via `num_runs`.
//
// For optimal timing resolution on modern x86 CPUs, you will want to tune
// `iters_per_run` so that the median run timing is around a few tens of µs:
// - The TSC CPU clock which is most likely used by your operating system's
//   high-resolution clock has an overhead of a few tens of CPU cycles, so for
//   a ~GHz CPU clock this gives you a clock-related bias and noise of ~10ns. At
//   ~10µs run times, that only contributes for 1/1000 of observed timings.
// - The main source of benchmark noise is that your operating system's
//   scheduler disturbs the program every few milliseconds. With run timings of
//   ~10µs, this disturbance affects less than 1% of data points, and is thus
//   perfectly eliminated by our outlier-robust statistics.
//
// As an extra, and unfortunately sometimes conflicting suggestion, it is
// advised to keep `iters_per_run` above 30, which is the classic rule of thumb
// for the validity of the central limit theorem. This allows you to safely
// assume that the random run timing distribution follows a roughly normal
// distribution, no matter what the actual underlying iteration timing
// probability law is, and the statistical analysis methods of
// `MicroBenchmarkResult` pervasively rely on this normality hypothesis. At
// lower `iters_per_run`, please cross-check yourself that your run timings
// follow a normal distribution, and use a statistical analysis methodology that
// is appropriate for the run timings distribution otherwise.
//
// You shouldn't usually need to adjust the number of runs and warmup time, but
// here are some guidelines for those times when you need to:
// - `num_runs` is a somewhat delicate compromise between several concerns:
//       * Quality of error bars (many runs mean more precise error bars)
//       * Outlier rejection (many runs mean better outlier rejection)
//       * Benchmark running time (many runs take longer)
//       * Handling of short-lived background disturbances (these have a higher
//         chance of occurring in longer-lived benchmarks, but if they only take
//         a small portion of the benchmark's running time, they can be taken
//         care of by outlier rejection instead of polluting the results)
// - `warmup_time` should be chosen based on the time it takes for run timings
//   to reach a steady state on your system after a benchmark starts. Here is a
//   possible measurement protocol for that:
//       * Set up a long-running benchmark (several seconds) with no warmup.
//       * Dump benchmark run timings to a file at the end of every execution.
//       * Run this benchmark a couple of times (say, 5-10x times).
//       * Plot the resulting run timing time series against their cumulated sum
//         (which is a good approximation of the elapsed time).
//       * Note after how much elapsed time the timings typically become steady.
//

template <typename Callable>
MicroBenchmarkResult microBenchmark(
    Callable&& iteration, std::size_t iters_per_run,
    std::size_t num_runs = 20000,
    std::chrono::milliseconds warmup_time = std::chrono::milliseconds(2000)) {
  return benchmark_tools_internal::microBenchmarkImpl(
      [&] {
        for (std::size_t iter = 0; iter < iters_per_run; ++iter) {
          benchmark_tools_internal::MicroBenchmarkIter<Callable>::iter(
              iteration);
        }
      },
      iters_per_run, num_runs, warmup_time);
}

// Same idea as above, but the iteration function takes one argument, which is
// taken from the `inputs` collection. A run is one iteration through `inputs`.
//
// This variant is convenient when you want to test on random data in order to
// avoid the bias of always benchmarking on the same data, but don't want to
// "see" the cost of random number generation in your benchmark timings.
template <typename Callable, typename Input>
MicroBenchmarkResult microBenchmark(
    Callable&& iterationWithInput, const std::vector<Input>& inputs,
    std::size_t num_runs = 20000,
    std::chrono::milliseconds warmup_time = std::chrono::milliseconds(2000)) {
  return benchmark_tools_internal::microBenchmarkImpl(
      [&] {
        for (const auto& input : inputs) {
          benchmark_tools_internal::MicroBenchmarkIter<Callable, Input>::iter(
              iterationWithInput, &input);
        }
      },
      inputs.size(), num_runs, warmup_time);
}

}  // namespace Acts::Test
