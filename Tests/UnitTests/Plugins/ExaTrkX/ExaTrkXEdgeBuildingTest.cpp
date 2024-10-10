// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/ExaTrkX/detail/CantorEdge.hpp"
#include "Acts/Plugins/ExaTrkX/detail/TensorVectorConversion.hpp"
#include "Acts/Plugins/ExaTrkX/detail/buildEdges.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

#include <Eigen/Core>
#include <torch/torch.h>

using CantorPair = Acts::detail::CantorEdge<int>;

#define PRINT 0

float distance(const at::Tensor &a, const at::Tensor &b) {
  assert(a.sizes() == b.sizes());
  assert(a.sizes().size() == 1);

  return std::sqrt(((a - b) * (a - b)).sum().item().to<float>());
}

#if PRINT
std::ostream &operator<<(std::ostream &os, CantorPair p) {
  auto [a, b] = p.inverse();
  os << "(" << a << "," << b << ")";
  return os;
}
#endif

template <typename edge_builder_t>
void test_random_graph(int emb_dim, int n_nodes, float r, int knn,
                       const edge_builder_t &edgeBuilder) {
  // Create a random point cloud
  auto random_features = at::randn({n_nodes, emb_dim});

  // Generate the truth via brute-force
  Eigen::MatrixXf distance_matrix(n_nodes, n_nodes);

  std::vector<CantorPair> edges_ref_cantor;
  std::vector<int> edge_counts(n_nodes, 0);

  for (int i = 0; i < n_nodes; ++i) {
    for (int j = i; j < n_nodes; ++j) {
      const auto d = distance(random_features[i], random_features[j]);
      distance_matrix(i, j) = d;
      distance_matrix(j, i) = d;

      if (d < r && i != j) {
        edges_ref_cantor.emplace_back(i, j);
        edge_counts[i]++;
      }
    }
  }

  const auto max_edges =
      *std::max_element(edge_counts.begin(), edge_counts.end());

  // If this is not the case, the test is ill-formed
  // knn specifies how many edges can be found by the function at max. Thus, we
  // should design the test in a way, that our brute-force test algorithm does
  // not find more edges than the algorithm that we test against it can find
  BOOST_REQUIRE(max_edges <= knn);

  // Run the edge building
  auto edges_test = edgeBuilder(random_features, r, knn);

  // Map the edges to cantor pairs
  std::vector<CantorPair> edges_test_cantor;

  for (int i = 0; i < edges_test.size(1); ++i) {
    const auto a = edges_test[0][i].template item<int>();
    const auto b = edges_test[1][i].template item<int>();
    edges_test_cantor.push_back(a < b ? CantorPair(a, b) : CantorPair(b, a));
  }

  std::ranges::sort(edges_ref_cantor, std::less<CantorPair>{});
  std::ranges::sort(edges_test_cantor, std::less<CantorPair>{});

#if PRINT
  std::cout << "test size " << edges_test_cantor.size() << std::endl;
  std::cout << "ref size " << edges_ref_cantor.size() << std::endl;
  std::cout << "test: ";
  std::copy(
      edges_test_cantor.begin(),
      edges_test_cantor.begin() + std::min(edges_test_cantor.size(), 10ul),
      std::ostream_iterator<CantorPair>(std::cout, " "));
  std::cout << std::endl;
  std::cout << "ref: ";
  std::copy(edges_ref_cantor.begin(),
            edges_ref_cantor.begin() + std::min(edges_ref_cantor.size(), 10ul),
            std::ostream_iterator<CantorPair>(std::cout, " "));
  std::cout << std::endl;
#endif

  // Check
  BOOST_CHECK_EQUAL(edges_ref_cantor.size(), edges_test_cantor.size());
  BOOST_CHECK(std::equal(edges_test_cantor.begin(), edges_test_cantor.end(),
                         edges_ref_cantor.begin()));
}

BOOST_AUTO_TEST_CASE(test_cantor_pair_functions) {
  int a = 345;
  int b = 23;
  // Use non-sorted cantor pair to make this work
  const auto [aa, bb] = CantorPair(a, b, false).inverse();
  BOOST_CHECK_EQUAL(a, aa);
  BOOST_CHECK_EQUAL(b, bb);
}

BOOST_AUTO_TEST_CASE(test_cantor_pair_sorted) {
  int a = 345;
  int b = 23;
  CantorPair c1(a, b);
  CantorPair c2(b, a);
  BOOST_CHECK_EQUAL(c1.value(), c2.value());
}

const int emb_dim = 3;
const int n_nodes = 20;
const float r = 1.5;
const int knn = 50;
const int seed = 42;

BOOST_AUTO_TEST_CASE(test_random_graph_edge_building_cuda,
                     *boost::unit_test::precondition([](auto) {
                       return torch::cuda::is_available();
                     })) {
  torch::manual_seed(seed);

  auto cudaEdgeBuilder = [](auto &features, auto radius, auto k) {
    auto features_cuda = features.to(torch::kCUDA);
    return Acts::detail::buildEdgesFRNN(features_cuda, radius, k);
  };

  test_random_graph(emb_dim, n_nodes, r, knn, cudaEdgeBuilder);
}

BOOST_AUTO_TEST_CASE(test_random_graph_edge_building_kdtree) {
  torch::manual_seed(seed);

  auto cpuEdgeBuilder = [](auto &features, auto radius, auto k) {
    auto features_cpu = features.to(torch::kCPU);
    return Acts::detail::buildEdgesKDTree(features_cpu, radius, k);
  };

  test_random_graph(emb_dim, n_nodes, r, knn, cpuEdgeBuilder);
}

BOOST_AUTO_TEST_CASE(test_self_loop_removal) {
  // clang-format off
  std::vector<std::int64_t> edges = {
    1,1,
    2,3,
    2,2,
    5,4,
  };
  // clang-format on

  auto opts = torch::TensorOptions().dtype(torch::kInt64);
  const auto edgeTensor =
      torch::from_blob(edges.data(), {static_cast<long>(edges.size() / 2), 2},
                       opts)
          .transpose(0, 1);

  const auto withoutSelfLoops =
      Acts::detail::postprocessEdgeTensor(edgeTensor, true, false, false)
          .transpose(1, 0)
          .flatten();

  const std::vector<std::int64_t> postEdges(
      withoutSelfLoops.data_ptr<std::int64_t>(),
      withoutSelfLoops.data_ptr<std::int64_t>() + withoutSelfLoops.numel());

  // clang-format off
  const std::vector<std::int64_t> ref = {
    2,3,
    5,4,
  };
  // clang-format on

  BOOST_CHECK_EQUAL(ref, postEdges);
}

BOOST_AUTO_TEST_CASE(test_duplicate_removal) {
  // clang-format off
  std::vector<std::int64_t> edges = {
    1,2,
    2,1,   // duplicate, flipped
    3,2,
    3,2,   // duplicate, not flipped
    7,6,   // should be flipped
  };
  // clang-format on

  auto opts = torch::TensorOptions().dtype(torch::kInt64);
  const auto edgeTensor =
      torch::from_blob(edges.data(), {static_cast<long>(edges.size() / 2), 2},
                       opts)
          .transpose(0, 1);

  const auto withoutDups =
      Acts::detail::postprocessEdgeTensor(edgeTensor, false, true, false)
          .transpose(1, 0)
          .flatten();

  const std::vector<std::int64_t> postEdges(
      withoutDups.data_ptr<std::int64_t>(),
      withoutDups.data_ptr<std::int64_t>() + withoutDups.numel());

  // clang-format off
  const std::vector<std::int64_t> ref = {
    1,2,
    2,3,
    6,7,
  };
  // clang-format on

  BOOST_CHECK_EQUAL(ref, postEdges);
}

BOOST_AUTO_TEST_CASE(test_random_flip) {
  torch::manual_seed(seed);

  // clang-format off
  std::vector<std::int64_t> edges = {
    1,2,
    2,3,
    3,4,
    4,5,
  };
  // clang-format on

  auto opts = torch::TensorOptions().dtype(torch::kInt64);
  const auto edgeTensor =
      torch::from_blob(edges.data(), {static_cast<long>(edges.size() / 2), 2},
                       opts)
          .transpose(0, 1);

  const auto flipped =
      Acts::detail::postprocessEdgeTensor(edgeTensor, false, false, true)
          .transpose(0, 1)
          .flatten();

  const std::vector<std::int64_t> postEdges(
      flipped.data_ptr<std::int64_t>(),
      flipped.data_ptr<std::int64_t>() + flipped.numel());

  BOOST_CHECK_EQUAL(postEdges.size(), edges.size());
  for (auto preIt = edges.begin(); preIt != edges.end(); preIt += 2) {
    int found = 0;

    for (auto postIt = postEdges.begin(); postIt != postEdges.end();
         postIt += 2) {
      bool noflp = (*preIt == *postIt) and *(preIt + 1) == *(postIt + 1);
      bool flp = *preIt == *(postIt + 1) and *(preIt + 1) == *(postIt);

      found += (flp or noflp);
    }

    BOOST_CHECK_EQUAL(found, 1);
  }
}
