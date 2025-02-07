// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include <Acts/Plugins/ExaTrkX/detail/ConnectedComponents.cuh>

#include <filesystem>
#include <fstream>
#include <random>
#include <set>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>

using namespace Acts::detail;

using BoostGraph =
    boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS>;

using Vi = std::vector<int>;

Vi checkLabeling(const std::vector<int> &src, const std::vector<int> &tgt) {
  std::size_t numNodes = std::max(*std::max_element(src.begin(), src.end()),
                                  *std::max_element(tgt.begin(), tgt.end())) +
                         1;

  int *cudaSrc, *cudaTgt;
  BOOST_REQUIRE_EQUAL(cudaMalloc(&cudaSrc, src.size() * sizeof(int)),
                      cudaSuccess);
  BOOST_REQUIRE_EQUAL(cudaMalloc(&cudaTgt, tgt.size() * sizeof(int)),
                      cudaSuccess);
  BOOST_REQUIRE_EQUAL(cudaMemcpy(cudaSrc, src.data(), src.size() * sizeof(int),
                                 cudaMemcpyHostToDevice),
                      cudaSuccess);
  BOOST_REQUIRE_EQUAL(cudaMemcpy(cudaTgt, tgt.data(), src.size() * sizeof(int),
                                 cudaMemcpyHostToDevice),
                      cudaSuccess);

  int *cudaLabels;
  BOOST_REQUIRE_EQUAL(cudaMalloc(&cudaLabels, numNodes * sizeof(int)),
                      cudaSuccess);
  int *cudaLabelsNext;
  BOOST_REQUIRE_EQUAL(cudaMalloc(&cudaLabelsNext, numNodes * sizeof(int)),
                      cudaSuccess);

  labelConnectedComponents<<<1, 1024>>>(src.size(), cudaSrc, cudaTgt, numNodes,
                                        cudaLabels, cudaLabelsNext);

  std::vector<int> labelsFromCuda(numNodes);
  BOOST_REQUIRE_EQUAL(
      cudaMemcpy(labelsFromCuda.data(), cudaLabels, numNodes * sizeof(int),
                 cudaMemcpyDeviceToHost),
      cudaSuccess);

  BoostGraph G(numNodes);

  for (auto i = 0ul; i < src.size(); ++i) {
    boost::add_edge(src[i], tgt[i], G);
  }

  std::vector<std::size_t> cpuLabels(numNodes);
  boost::connected_components(G, &cpuLabels[0]);

  // print
  std::cout << "cpu labels:     ";
  for (auto i = 0ul; i < numNodes; ++i) {
    std::cout << cpuLabels[i] << " ";
  }
  std::cout << std::endl;

  std::cout << "my CUDA labels: ";
  for (auto i = 0ul; i < numNodes; ++i) {
    std::cout << labelsFromCuda[i] << " ";
  }
  std::cout << std::endl;

  // check systematically
  std::map<int, int> boostToCuda;
  for (auto i = 0ul; i < numNodes; ++i) {
    if (boostToCuda.contains(cpuLabels[i])) {
      BOOST_CHECK_EQUAL(labelsFromCuda[i], boostToCuda.at(cpuLabels[i]));
    } else {
      auto [it, success] =
          boostToCuda.insert({cpuLabels[i], labelsFromCuda[i]});
      BOOST_CHECK(success);
    }
  }

  return labelsFromCuda;
}

BOOST_AUTO_TEST_CASE(simple_test_1) {
  Vi src{0, 1, 2, 3};
  Vi tgt{1, 2, 3, 4};
  checkLabeling(src, tgt);
}

BOOST_AUTO_TEST_CASE(simple_test_2) {
  Vi src{0, 1, 2, 4, 5, 6};
  Vi tgt{1, 2, 3, 5, 6, 7};
  checkLabeling(src, tgt);
}

BOOST_AUTO_TEST_CASE(simple_test_3) {
  Vi src{4, 3, 2, 1};
  Vi tgt{3, 2, 1, 0};
  checkLabeling(src, tgt);
}

void testRelabeling(const Vi &labels, const Vi &refLabelMask,
                    const Vi &refPrefixSum, const Vi &refLabels) {
  dim3 blockDim = 32;
  dim3 gridDim = (labels.size() + blockDim.x - 1) / blockDim.x;

  // Copy labels to device
  int *cudaLabels;
  BOOST_REQUIRE_EQUAL(cudaMalloc(&cudaLabels, labels.size() * sizeof(int)),
                      cudaSuccess);
  BOOST_REQUIRE_EQUAL(
      cudaMemcpy(cudaLabels, labels.data(), labels.size() * sizeof(int),
                 cudaMemcpyHostToDevice),
      cudaSuccess);

  // Init label mask
  int *cudaLabelMask;
  BOOST_REQUIRE_EQUAL(cudaMalloc(&cudaLabelMask, labels.size() * sizeof(int)),
                      cudaSuccess);
  BOOST_REQUIRE_EQUAL(cudaMemset(cudaLabelMask, 0, labels.size() * sizeof(int)),
                      cudaSuccess);

  makeLabelMask<<<1, 256>>>(labels.size(), cudaLabels, cudaLabelMask);
  BOOST_REQUIRE_EQUAL(cudaDeviceSynchronize(), cudaSuccess);

  std::vector<int> labelMask(labels.size());
  BOOST_REQUIRE_EQUAL(
      cudaMemcpy(labelMask.data(), cudaLabelMask,
                 labelMask.size() * sizeof(int), cudaMemcpyDeviceToHost),
      cudaSuccess);

  BOOST_CHECK_EQUAL_COLLECTIONS(labelMask.begin(), labelMask.end(),
                                refLabelMask.begin(), refLabelMask.end());

  // Prefix sum
  int *cudaPrefixSum;
  BOOST_REQUIRE_EQUAL(cudaMalloc(&cudaPrefixSum, labels.size() * sizeof(int)),
                      cudaSuccess);
  thrust::exclusive_scan(thrust::device.on(0), cudaLabelMask,
                         cudaLabelMask + labels.size(), cudaPrefixSum);

  Vi prefixSum(labels.size());
  BOOST_REQUIRE_EQUAL(
      cudaMemcpy(prefixSum.data(), cudaPrefixSum, labels.size() * sizeof(int),
                 cudaMemcpyDeviceToHost),
      cudaSuccess);
  BOOST_CHECK_EQUAL_COLLECTIONS(prefixSum.begin(), prefixSum.end(),
                                refPrefixSum.begin(), refPrefixSum.end());

  // Relabel
  mapEdgeLabels<<<1, 256>>>(labels.size(), cudaLabels, cudaPrefixSum);
  BOOST_REQUIRE_EQUAL(cudaDeviceSynchronize(), cudaSuccess);

  std::vector<int> labelsFromCuda(labels.size());
  BOOST_REQUIRE_EQUAL(
      cudaMemcpy(labelsFromCuda.data(), cudaLabels, labels.size() * sizeof(int),
                 cudaMemcpyDeviceToHost),
      cudaSuccess);

  BOOST_CHECK_EQUAL_COLLECTIONS(labelsFromCuda.begin(), labelsFromCuda.end(),
                                refLabels.begin(), refLabels.end());
}

BOOST_AUTO_TEST_CASE(test_relabeling) {
  // clang-format off
  Vi labels      {0, 3, 5, 3, 0, 0};
  Vi refLabelMask{1, 0, 0, 1, 0, 1};
  Vi refPrefixSum{0, 1, 1, 1, 2, 2};
  Vi refLabels   {0, 1, 2, 1, 0, 0};
  // clang-format on

  testRelabeling(labels, refLabelMask, refPrefixSum, refLabels);
}

BOOST_AUTO_TEST_CASE(test_relabeling_2) {
  // clang-format off
  Vi labels      {1, 3, 5, 3, 1, 1};
  Vi refLabelMask{0, 1, 0, 1, 0, 1};
  Vi refPrefixSum{0, 0, 1, 1, 2, 2};
  Vi refLabels   {0, 1, 2, 1, 0, 0};
  // clang-format on

  testRelabeling(labels, refLabelMask, refPrefixSum, refLabels);
}

auto makeRandomGraph(std::size_t nodes, std::size_t edges) {
  std::default_random_engine rng(2345);
  std::uniform_int_distribution<> dist(0, nodes);
  std::set<std::pair<int, int>> set;
  Vi src(edges), tgt(edges);
  for (auto n = 0ul; n < edges; ++n) {
    auto a = dist(rng);
    auto b = dist(rng);
    if (a == b) {
      continue;
    }
    auto s = std::min(a, b);
    auto t = std::max(a, b);
    auto [it, success] = set.insert({s, t});
    if (success) {
      src.at(n) = s;
      tgt.at(n) = t;
    }
  }

  return std::make_pair(src, tgt);
}

BOOST_AUTO_TEST_CASE(test_random_graph) {
  auto [src, tgt] = makeRandomGraph(5, 10);
  checkLabeling(src, tgt);
}

void testFullConnectedComponents(const Vi &src, const Vi &tgt) {
  const auto nNodes = std::max(*std::max_element(src.begin(), src.end()),
                               *std::max_element(tgt.begin(), tgt.end())) +
                      1;

  // print src and tgt
  /*
    std::cout << "src: ";
    for (int i = 0; i < src.size(); ++i) {
      std::cout << src[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "tgt: ";
    for (int i = 0; i < tgt.size(); ++i) {
      std::cout << tgt[i] << " ";
    }
    std::cout << std::endl;
  */
  cudaStream_t stream;
  BOOST_REQUIRE_EQUAL(cudaStreamCreate(&stream), cudaSuccess);

  // copy src and tgt to device
  int *cudaSrc, *cudaTgt;
  BOOST_REQUIRE_EQUAL(
      cudaMallocAsync(&cudaSrc, src.size() * sizeof(int), stream), cudaSuccess);
  BOOST_REQUIRE_EQUAL(
      cudaMallocAsync(&cudaTgt, tgt.size() * sizeof(int), stream), cudaSuccess);
  BOOST_REQUIRE_EQUAL(
      cudaMemcpyAsync(cudaSrc, src.data(), src.size() * sizeof(int),
                      cudaMemcpyHostToDevice, stream),
      cudaSuccess);
  BOOST_REQUIRE_EQUAL(
      cudaMemcpyAsync(cudaTgt, tgt.data(), src.size() * sizeof(int),
                      cudaMemcpyHostToDevice, stream),
      cudaSuccess);

  // init label array
  int *cudaLabels;
  BOOST_REQUIRE_EQUAL(
      cudaMallocAsync(&cudaLabels, nNodes * sizeof(int), stream), cudaSuccess);

  // run connected components
  int cudaNumLabels = connectedComponentsCuda(src.size(), cudaSrc, cudaTgt,
                                              nNodes, cudaLabels, stream);
  BOOST_REQUIRE_EQUAL(cudaStreamSynchronize(stream), cudaSuccess);

  // print message from last cuda error code
  std::cout << "CUDA Error msg: " << cudaGetErrorString(cudaPeekAtLastError())
            << std::endl;
  BOOST_REQUIRE_EQUAL(cudaGetLastError(), cudaSuccess);

  // copy labels back
  std::vector<int> labelsFromCuda(nNodes);
  BOOST_REQUIRE_EQUAL(
      cudaMemcpyAsync(labelsFromCuda.data(), cudaLabels, nNodes * sizeof(int),
                      cudaMemcpyDeviceToHost, stream),
      cudaSuccess);

  BOOST_REQUIRE_EQUAL(cudaFreeAsync(cudaSrc, stream), cudaSuccess);
  BOOST_REQUIRE_EQUAL(cudaFreeAsync(cudaTgt, stream), cudaSuccess);
  BOOST_REQUIRE_EQUAL(cudaFreeAsync(cudaLabels, stream), cudaSuccess);

  // sync
  BOOST_REQUIRE_EQUAL(cudaStreamSynchronize(stream), cudaSuccess);
  BOOST_REQUIRE_EQUAL(cudaStreamDestroy(stream), cudaSuccess);

  // print labelsFromCuda
  /*
      std::cout << "CUDA labels: ";
      for (int i = 0; i < nNodes; ++i) {
        std::cout << labelsFromCuda[i] << " ";
      }
      std::cout << std::endl;
  */
  // run boost graph for comparison

  BoostGraph G(nNodes);

  for (auto i = 0ul; i < src.size(); ++i) {
    boost::add_edge(src[i], tgt[i], G);
  }

  std::vector<std::size_t> cpuLabels(boost::num_vertices(G));
  int cpuNumLabels = boost::connected_components(G, &cpuLabels[0]);

  // check
  BOOST_CHECK_EQUAL(cudaNumLabels, cpuNumLabels);
  BOOST_CHECK_EQUAL_COLLECTIONS(labelsFromCuda.begin(), labelsFromCuda.end(),
                                cpuLabels.begin(), cpuLabels.end());
}

BOOST_AUTO_TEST_CASE(full_test_tiny_graph) {
  auto [src, tgt] = makeRandomGraph(5, 10);
  testFullConnectedComponents(src, tgt);
}

BOOST_AUTO_TEST_CASE(full_test_small_graph) {
  auto [src, tgt] = makeRandomGraph(100, 500);
  testFullConnectedComponents(src, tgt);
}

BOOST_AUTO_TEST_CASE(full_test_big_graph) {
  for (int i = 0; i < 3; ++i) {
    std::cout << "Test graph " << i << std::endl;
    auto [src, tgt] = makeRandomGraph(100'000, 500'000);
    testFullConnectedComponents(src, tgt);
  }
}

BOOST_AUTO_TEST_CASE(test_from_file) {
  if (!std::filesystem::exists("edges_cuda_trackbuilding.txt")) {
    std::cout << "File edges_cuda_trackbuilding.txt not found" << std::endl;
    return;
  }

  std::ifstream file("edges_cuda_trackbuilding.txt");
  std::vector<int> src, tgt;
  int a, b;
  while (file >> a >> b) {
    src.push_back(a);
    tgt.push_back(b);
  }

  testFullConnectedComponents(src, tgt);
}

// try this pathologic case
BOOST_AUTO_TEST_CASE(special_1) {
  testFullConnectedComponents({1, 2}, {4, 7});
}

BOOST_AUTO_TEST_CASE(special_2) {
  Vi src{1, 2};
  Vi tgt{4, 7};
  checkLabeling(src, tgt);
}

BOOST_AUTO_TEST_CASE(special_3) {
  // clang-format off
  Vi labels      {0, 1, 2, 3, 1, 5, 6, 2};
  Vi refLabelMask{1, 1, 1, 1, 0, 1, 1, 0};
  Vi refPrefixSum{0, 1, 2, 3, 4, 4, 5, 6};
  Vi refLabels   {0, 1, 2, 3, 1, 4, 5, 2};
  // clang-format on

  testRelabeling(labels, refLabelMask, refPrefixSum, refLabels);
}

BOOST_AUTO_TEST_CASE(special_4) {
  Vi src{1, 2};
  Vi tgt{4, 7};

  auto labelsFromCuda = checkLabeling(src, tgt);

  Vi refLabelMask{1, 1, 1, 1, 0, 1, 1, 0};
  Vi refPrefixSum{0, 1, 2, 3, 4, 4, 5, 6};
  Vi refLabels{0, 1, 2, 3, 1, 4, 5, 2};

  testRelabeling(labelsFromCuda, refLabelMask, refPrefixSum, refLabels);
}
