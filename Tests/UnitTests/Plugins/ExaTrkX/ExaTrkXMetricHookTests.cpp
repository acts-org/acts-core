// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/ExaTrkX/TorchTruthGraphMetricsHook.hpp"

#include <cassert>
#include <iostream>

#include <torch/torch.h>

void testTruthTestGraph(std::vector<std::int64_t> &truthGraph,
                        std::vector<std::int64_t> &testGraph,
                        const std::string &resStr) {
  std::stringstream ss;
  auto logger = Acts::getDefaultLogger("Test", Acts::Logging::INFO, &ss);

  Acts::TorchTruthGraphMetricsHook hook(truthGraph, std::move(logger));

  auto opts = torch::TensorOptions().dtype(torch::kInt64);
  const auto edgeTensor =
      torch::from_blob(testGraph.data(),
                       {static_cast<long>(testGraph.size() / 2), 2}, opts)
          .transpose(0, 1);

  hook({}, edgeTensor, {});

  const auto str = ss.str();

  auto begin = str.begin() + str.find("Efficiency");
  BOOST_CHECK_EQUAL(std::string(begin, str.end() - 1), resStr);
}

BOOST_AUTO_TEST_CASE(same_graph) {
  // clang-format off
  std::vector<std::int64_t> truthGraph = {
    1,2,
    2,3,
    3,4,
    4,5,
  };
  // clang-format on

  // clang-format off
  std::vector<std::int64_t> testGraph = {
    3,4,
    4,5,
    1,2,
    2,3,
  };
  // clang-format on

  testTruthTestGraph(truthGraph, testGraph, "Efficiency=1, purity=1");
}

// Test large numbers because overflows are a danger for cantor pairing
BOOST_AUTO_TEST_CASE(same_graph_large_numbers) {
  // clang-format off
  std::int64_t k = 100'000;

  std::vector<std::int64_t> truthGraph = {
    1,2,
    2,3,
    3,4,
    4,5,
  };
  // clang-format on
  std::transform(truthGraph.begin(), truthGraph.end(), truthGraph.begin(),
                 [&](auto i) { return k + i; });

  // clang-format off
  std::vector<std::int64_t> testGraph = {
    3,4,
    4,5,
    1,2,
    2,3,
  };
  // clang-format on
  std::transform(testGraph.begin(), testGraph.end(), testGraph.begin(),
                 [&](auto i) { return k + i; });

  testTruthTestGraph(truthGraph, testGraph, "Efficiency=1, purity=1");
}

BOOST_AUTO_TEST_CASE(fifty_fifty) {
  // clang-format off
  std::vector<std::int64_t> truthGraph = {
    1,2,
    2,3,
    3,4,
    4,5,
  };
  // clang-format on

  // clang-format off
  std::vector<std::int64_t> testGraph = {
    3,4,
    4,5,
    6,9,
    5,1,
  };
  // clang-format on

  testTruthTestGraph(truthGraph, testGraph, "Efficiency=0.5, purity=0.5");
}
