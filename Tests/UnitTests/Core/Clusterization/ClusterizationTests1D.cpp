// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Clusterization/Clusterization.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

namespace Acts::Test {

struct Cell1D {
  Cell1D(int colv) : col(colv) {}
  int col;
  Ccl::Label label{Ccl::NO_LABEL};
};

bool cellComp(const Cell1D& left, const Cell1D& right) {
  return left.col < right.col;
}

Ccl::Label& getCellLabel(Cell1D& cell) {
  return cell.label;
}

int getCellColumn(const Cell1D& cell) {
  return cell.col;
}

struct Cluster1D {
  std::vector<Cell1D> cells;
  std::size_t hash{};
};

void clusterAddCell(Cluster1D& cl, const Cell1D& cell) {
  cl.cells.push_back(cell);
}

bool clHashComp(const Cluster1D& left, const Cluster1D& right) {
  return left.hash < right.hash;
}

void hash(Cluster1D& cl) {
  std::ranges::sort(cl.cells, cellComp);
  cl.hash = 0;
  for (const Cell1D& c : cl.cells) {
    boost::hash_combine(cl.hash, c.col);
  }
}

BOOST_AUTO_TEST_CASE(Grid_1D_rand) {
  using Cell = Cell1D;
  using CellC = std::vector<Cell>;
  using Cluster = Cluster1D;
  using ClusterC = std::vector<Cluster>;

  std::size_t minsize = 1;
  std::size_t maxsize = 10;
  std::size_t minspace = 1;
  std::size_t maxspace = 10;
  std::size_t nclusters = 100;
  int startSeed = 204769;
  std::size_t ntries = 100;

  std::cout << "Grid_1D_rand test with parameters:" << std::endl;
  std::cout << "  minsize = " << minsize << std::endl;
  std::cout << "  maxsize = " << maxsize << std::endl;
  std::cout << "  minspace = " << minspace << std::endl;
  std::cout << "  maxspace = " << maxspace << std::endl;
  std::cout << "  nclusters = " << nclusters << std::endl;
  std::cout << "  startSeed = " << startSeed << std::endl;
  std::cout << "  ntries = " << ntries << std::endl;

  while (ntries-- > 0) {
    std::mt19937_64 rnd(startSeed++);
    std::uniform_int_distribution<std::uint32_t> distr_size(minsize, maxsize);
    std::uniform_int_distribution<std::uint32_t> distr_space(minspace,
                                                             maxspace);

    int col = 0;

    CellC cells;
    ClusterC clusters;
    for (std::size_t i = 0; i < nclusters; i++) {
      Cluster cl;
      col += distr_space(rnd);
      std::uint32_t size = distr_size(rnd);
      for (std::uint32_t j = 0; j < size; j++) {
        Cell cell(col++);
        cells.push_back(cell);
        clusterAddCell(cl, cell);
      }
      clusters.push_back(std::move(cl));
    }
    for (Cluster& cl : clusters) {
      hash(cl);
    }

    std::shuffle(cells.begin(), cells.end(), rnd);

    ClusterC newCls = Ccl::createClusters<CellC, ClusterC, 1>(cells);

    for (Cluster& cl : newCls) {
      hash(cl);
    }

    std::ranges::sort(clusters, clHashComp);
    std::ranges::sort(newCls, clHashComp);

    BOOST_CHECK_EQUAL(clusters.size(), newCls.size());
    for (std::size_t i = 0; i < clusters.size(); i++) {
      BOOST_CHECK_EQUAL(clusters.at(i).hash, newCls.at(i).hash);
    }
  }
}

}  // namespace Acts::Test
