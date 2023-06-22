// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/ExaTrkX/buildEdges.hpp"
#include "Acts/Utilities/ContainerPrinter.hpp"

#include <cassert>
#include <iostream>

#include <Eigen/Core>
#include <torch/torch.h>

#define PRINT 0

bool haveCuda(boost::unit_test::test_unit_id) {
  return torch::cuda::is_available();
}

float distance(const at::Tensor &a, const at::Tensor &b) {
  assert(a.sizes() == b.sizes());
  assert(a.sizes().size() == 1);

  return std::sqrt(((a - b) * (a - b)).sum().item().to<float>());
}

struct CantorPair{
  int value;
  
  CantorPair(int x, int y) : value(y + ((x + y) * (x + y + 1)) / 2) {}
  
  std::pair<int, int> inverse() const {
    auto f = [](int w) -> int { return (w * (w + 1)) / 2; };
    auto q = [](int w) -> int {
      return std::floor((std::sqrt(8 * w + 1) - 1) / 2);
    };

    auto y = value - f(q(value));
    auto x = q(value) - y;

    return {x, y};
  }
};

#if PRINT
std::ostream &operator<<(std::ostream &os, CantorPair p) {
  auto [a, b] = p.inverse();
  os << "(" << a << "," << b << ")";
  return os;
}
#endif

bool operator<(CantorPair a, CantorPair b) {
  return a.value < b.value;
}

bool operator==(CantorPair a, CantorPair b) {
  return a.value == b.value;
}

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
  // knn specifies how many edges can be found by the function at max. Thus we
  // should design the test in a way, that our brute-force test algorithm does
  // not find more edges then the algorithm that we test against it can find
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
  
  std::sort(edges_ref_cantor.begin(), edges_ref_cantor.end());
  std::sort(edges_test_cantor.begin(), edges_test_cantor.end());
  
#if PRINT
  std::cout << "test size " << edges_test_cantor.size() << std::endl;
  std::cout << "ref size " << edges_ref_cantor.size() << std::endl;
  std::cout << "test: " << Acts::ContainerPrinter(edges_test_cantor, 10) << std::endl;
  std::cout << "ref: " << Acts::ContainerPrinter(edges_ref_cantor, 10) << std::endl;
#endif

  // Check
  BOOST_CHECK(edges_ref_cantor.size() == edges_test_cantor.size());
  BOOST_CHECK(std::equal(edges_test_cantor.begin(), edges_test_cantor.end(),
                         edges_ref_cantor.begin()));
}

BOOST_AUTO_TEST_CASE(test_cantor_pair_functions) {
  int a = 345;
  int b = 23;
  const auto [aa, bb] = CantorPair(a, b).inverse();
  BOOST_CHECK(a == aa);
  BOOST_CHECK(b == bb);
}

const int emb_dim = 3;
const int n_nodes = 20;
const float r = 1.5;
const int knn = 50;
const int seed = 42;

BOOST_AUTO_TEST_CASE(test_random_graph_edge_building_cuda, * boost::unit_test::precondition(haveCuda)) {
  torch::manual_seed(seed);

  auto cudaEdgeBuilder = [](auto &features, auto radius,
                            auto k) {
    auto features_cuda = features.to(torch::kCUDA);
    return Acts::detail::buildEdgesFRNN(features_cuda, radius, k);
  };

  test_random_graph(emb_dim, n_nodes, r, knn, cudaEdgeBuilder);
}

BOOST_AUTO_TEST_CASE(test_random_graph_edge_building_kdtree) {
  torch::manual_seed(seed);

  auto cpuEdgeBuilder = [](auto &features, auto radius,
                            auto k) {
    auto features_cpu = features.to(torch::kCPU);
    return Acts::detail::buildEdgesKDTree(features_cpu, radius, k);
  };

  test_random_graph(emb_dim, n_nodes, r, knn, cpuEdgeBuilder);
}
