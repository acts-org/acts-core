// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/MagneticField/BFieldMapUtils.hpp"
#include "Acts/MagneticField/InterpolatedBFieldMap.hpp"
#include "Acts/MagneticField/SolenoidBField.hpp"
#include "Acts/Tests/CommonHelpers/BenchmarkTools.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <numbers>
#include <random>
#include <string>

using namespace Acts::UnitLiterals;

int main(int argc, char* argv[]) {
  std::size_t iters_map = 5e2;
  std::size_t iters_solenoid = 3;
  std::size_t runs_solenoid = 1000;
  if (argc >= 2) {
    iters_map = std::stoi(argv[1]);
  }
  if (argc >= 3) {
    iters_solenoid = std::stoi(argv[2]);
  }
  if (argc >= 4) {
    runs_solenoid = std::stoi(argv[3]);
  }

  const double L = 5.8_m;
  const double R = (2.56 + 2.46) * 0.5 * 0.5_m;
  const std::size_t nCoils = 1154;
  const double bMagCenter = 2_T;
  const std::size_t nBinsR = 150;
  const std::size_t nBinsZ = 200;

  double rMin = -0.1;
  double rMax = R * 2.;
  double zMin = 2 * (-L / 2.);
  double zMax = 2 * (L / 2.);

  Acts::SolenoidBField bSolenoidField({R, L, nCoils, bMagCenter});
  std::cout << "Building interpolated field map" << std::endl;
  auto bFieldMap = Acts::solenoidFieldMap({rMin, rMax}, {zMin, zMax},
                                          {nBinsR, nBinsZ}, bSolenoidField);
  Acts::MagneticFieldContext mctx{};

  std::minstd_rand rng;
  std::uniform_real_distribution<double> zDist(1.5 * (-L / 2.), 1.5 * L / 2.);
  std::uniform_real_distribution<double> rDist(0, R * 1.5);
  std::uniform_real_distribution<double> phiDist(-std::numbers::pi,
                                                 std::numbers::pi);
  auto genPos = [&]() -> Acts::Vector3 {
    const double z = zDist(rng), r = rDist(rng), phi = phiDist(rng);
    return {r * std::cos(phi), r * std::sin(phi), z};
  };

  std::ofstream os{"bfield_bench.csv"};

  auto csv = [&](const std::string& name, auto res) {
    os << name << "," << res.run_timings.size() << "," << res.iters_per_run
       << "," << res.totalTime().count() << "," << res.runTimeMedian().count()
       << "," << 1.96 * res.runTimeError().count() << ","
       << res.iterTimeAverage().count() << ","
       << 1.96 * res.iterTimeError().count();

    os << std::endl;
  };

  os << "name,runs,iters,total_time,run_time_median,run_time_error,iter_"
        "time_average,iter_time_error"
     << std::endl;

  // SolenoidBField lookup is so slow that the cost of generating a random field
  // lookup position is negligible in comparison...
  std::cout << "Benchmarking random SolenoidBField lookup: " << std::flush;
  const auto solenoid_result = Acts::Test::microBenchmark(
      [&] { return bSolenoidField.getField(genPos()); }, iters_solenoid,
      runs_solenoid);
  std::cout << solenoid_result << std::endl;
  csv("solenoid", solenoid_result);

  // ...but for interpolated B-field map, the overhead of a field lookup is
  // comparable to that of generating a random position, so we must be more
  // careful. Hence we do two microbenchmarks which represent a kind of
  // lower and upper bound on field lookup performance.
  //
  // - The first benchmark operates at constant position, so it measures only
  //   field lookup overhead but has unrealistically good cache locality. In
  //   that sense, it provides a lower bound of field lookup performance.
  std::cout << "Benchmarking interpolated field lookup: " << std::flush;
  const auto fixedPos = genPos();
  const auto map_fixed_nocache_result = Acts::Test::microBenchmark(
      [&] { return bFieldMap.getField(fixedPos); }, iters_map);
  std::cout << map_fixed_nocache_result << std::endl;
  csv("interp_nocache_fixed", map_fixed_nocache_result);

  std::cout << "Benchmarking random interpolated field lookup: " << std::flush;

  // - The second benchmark generates random positions, so it is biased by the
  //   cost of random position generation and has unrealistically bad cache
  //   locality, but provides an upper bound of field lookup performance.
  const auto map_rand_result = Acts::Test::microBenchmark(
      [&] { return bFieldMap.getField(genPos()); }, iters_map);
  std::cout << map_rand_result << std::endl;
  csv("interp_nocache_random", map_rand_result);

  // - This variation of the first benchmark uses a fixed position again, but
  //   uses the cache infrastructure to evaluate how much of an impact it has on
  //   performance in this scenario. We expect this to improve performance as
  //   the cache will always be valid for the fixed point.
  {
    std::cout << "Benchmarking cached interpolated field lookup: "
              << std::flush;
    auto cache = bFieldMap.makeCache(mctx);
    const auto map_cached_result_cache = Acts::Test::microBenchmark(
        [&] { return bFieldMap.getField(fixedPos, cache).value(); }, iters_map);
    std::cout << map_cached_result_cache << std::endl;
    csv("interp_cache_fixed", map_cached_result_cache);
  }

  // - This variation of the second benchmark again generates random positions
  //   and uses the cache infrastructure to evaluate the impact on performance.
  //   We expect this to deteriorate performance, as the cache will most likely
  //   be invalid and need to be recreated, on top of the underlying lookup.
  {
    std::cout << "Benchmarking cached random interpolated field lookup: "
              << std::flush;
    auto cache2 = bFieldMap.makeCache(mctx);
    const auto map_rand_result_cache = Acts::Test::microBenchmark(
        [&] { return bFieldMap.getField(genPos(), cache2).value(); },
        iters_map);
    std::cout << map_rand_result_cache << std::endl;
    csv("interp_cache_random", map_rand_result_cache);
  }

  // - The fourth benchmark tests a more 'realistic' access pattern than fixed
  //   or random positions: it advances along a straight line (which is close to
  //   a slightly curved line which happens in particle propagation). This
  //   instance does not use the cache infrastructure, so is effectively close
  //   to the random points benchmark, although positions are not really random.
  {
    std::cout << "Benchmarking advancing interpolated field lookup: "
              << std::flush;
    Acts::Vector3 pos{0, 0, 0};
    Acts::Vector3 dir{};
    dir.setRandom();
    double h = 1e-3;
    std::vector<Acts::Vector3> steps;
    steps.reserve(iters_map);
    for (std::size_t i = 0; i < iters_map; i++) {
      pos += dir * h;
      double z = pos[Acts::eFreePos2];
      if (Acts::VectorHelpers::perp(pos) > rMax || z >= zMax || z < zMin) {
        break;
      }
      steps.push_back(pos);
    }
    const auto map_adv_result = Acts::Test::microBenchmark(
        [&](const auto& s) { return bFieldMap.getField(s); }, steps);
    std::cout << map_adv_result << std::endl;
    csv("interp_nocache_adv", map_adv_result);

    // - This variation of the fourth benchmark advances in a straight line, but
    //   also uses the cache infrastructure. As subsequent positions are close
    //   to one another, the cache will be valid for a certain number of points,
    //   before becoming invalid. This means we expect performance to improve
    //   over the uncached straight line advance.

    std::cout << "Benchmarking cached advancing interpolated field lookup: "
              << std::flush;
    auto cache = bFieldMap.makeCache(mctx);
    const auto map_adv_result_cache = Acts::Test::microBenchmark(
        [&](const auto& s) { return bFieldMap.getField(s, cache).value(); },
        steps);
    std::cout << map_adv_result_cache << std::endl;
    csv("interp_cache_adv", map_adv_result_cache);
  }
}
