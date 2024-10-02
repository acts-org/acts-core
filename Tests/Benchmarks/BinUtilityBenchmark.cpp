// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Tests/CommonHelpers/BenchmarkTools.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <vector>

#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace Acts;

int main(int argc, char* argv[]) {
  unsigned int lvl = Acts::Logging::INFO;
  unsigned int toys = 1;

  try {
    po::options_description desc("Allowed options");
    // clang-format off
  desc.add_options()
      ("help", "produce help message")
      ("toys",po::value<unsigned int>(&toys)->default_value(100000000),"number searches to be done")
      ("verbose",po::value<unsigned int>(&lvl)->default_value(Acts::Logging::INFO),"logging level");
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help")) {
      std::cout << desc << std::endl;
      return 0;
    }
  } catch (std::exception& e) {
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
  }

  ACTS_LOCAL_LOGGER(getDefaultLogger("BinUtility", Acts::Logging::Level(lvl)));

  std::vector<float> fewBins;
  fewBins.reserve(6);
  for (unsigned int ib = 0; ib < 6; ++ib) {
    fewBins.push_back(ib * 6. / 5.);
  }
  Acts::BinUtility small(fewBins, Acts::open, Acts::BinningValue::binX);

  std::vector<float> mediumBins;
  mediumBins.reserve(21);
  for (unsigned int ib = 0; ib < 21; ++ib) {
    mediumBins.push_back(ib * 6. / 20.);
  }
  Acts::BinUtility medium(mediumBins, Acts::open, Acts::BinningValue::binX);

  std::vector<float> manyBins;
  manyBins.reserve(101);
  for (unsigned int ib = 0; ib < 101; ++ib) {
    manyBins.push_back(ib * 6. / 100.);
  }

  Acts::BinUtility many(manyBins, Acts::open, Acts::BinningValue::binX);

  Acts::Vector3 low = Acts::Vector3(1.5, 0., 0.);
  Acts::Vector3 high = Acts::Vector3(4.5, 0., 0.);

  std::size_t st = 0;
  std::size_t gt = 0;
  std::size_t num_iters = 0;
  const auto bin_utility_benchmark_small = Acts::Test::microBenchmark(
      [&] {
        auto bin = (num_iters % 2) != 0u ? small.bin(low) : small.bin(high);
        if (bin < 3) {
          ++st;
        } else {
          ++gt;
        }
        ++num_iters;
      },
      1, toys);

  ACTS_INFO("Execution stats small: " << bin_utility_benchmark_small);
  ACTS_INFO("Fraction is: " << st << " vs. " << gt);

  st = 0;
  gt = 0;
  num_iters = 0;
  const auto bin_utility_benchmark_medium = Acts::Test::microBenchmark(
      [&] {
        auto bin = (num_iters % 2) != 0u ? medium.bin(low) : medium.bin(high);
        if (bin < 10) {
          ++st;
        } else {
          ++gt;
        }
        ++num_iters;
      },
      1, toys);

  ACTS_INFO("Execution stats medium: " << bin_utility_benchmark_medium);
  ACTS_INFO("Fraction is: " << st << " vs. " << gt);

  st = 0;
  gt = 0;
  num_iters = 0;
  const auto bin_utility_benchmark_many = Acts::Test::microBenchmark(
      [&] {
        auto bin = (num_iters % 2) != 0u ? many.bin(low) : many.bin(high);
        if (bin < 49) {
          ++st;
        } else {
          ++gt;
        }
        ++num_iters;
      },
      1, toys);

  ACTS_INFO("Execution stats many: " << bin_utility_benchmark_many);
  ACTS_INFO("Fraction is: " << st << " vs. " << gt);

  Acts::BinUtility equidistant(100, 0., 6., Acts::open,
                               Acts::BinningValue::binX);
  st = 0;
  gt = 0;
  num_iters = 0;
  const auto bin_utility_benchmark_eq = Acts::Test::microBenchmark(
      [&] {
        auto bin = (num_iters % 2) != 0u ? equidistant.bin(low)
                                         : equidistant.bin(high);
        if (bin < 49) {
          ++st;
        } else {
          ++gt;
        }
        ++num_iters;
      },
      1, toys);

  ACTS_INFO("Execution stats equidistant: " << bin_utility_benchmark_eq);
  ACTS_INFO("Fraction is: " << st << " vs. " << gt);

  return 0;
}
