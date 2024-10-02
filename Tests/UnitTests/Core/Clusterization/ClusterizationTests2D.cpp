// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Clusterization/Clusterization.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

namespace Acts::Test {

using Rectangle = std::array<int, 4>;

std::vector<Rectangle> concat(std::vector<std::vector<Rectangle>> vecs) {
  std::vector<Rectangle> flat;
  for (std::vector<Rectangle>& v : vecs) {
    if (flat.empty()) {
      flat = std::move(v);
    } else {
      flat.reserve(flat.size() + v.size());
      std::move(v.begin(), v.end(), std::back_inserter(flat));
      v.clear();
    }
  }
  return flat;
}

template <typename RNG>
std::vector<Rectangle> segment(int x0, int y0, int x1, int y1, RNG& rng) {
  // Leave enough space for one-cell buffer around clusters
  int xmin = x0 + 3;
  int xmax = x1 - 3;
  int ymin = y0 + 3;
  int ymax = y1 - 3;

  // terminal case 1
  if (xmax < xmin || ymax < ymin) {
    return {{x0, y0, x1, y1}};
  }

  std::bernoulli_distribution cointoss;
  bool splitx = cointoss(rng);
  bool splity = cointoss(rng);

  // terminal case 2
  if (!(splitx || splity)) {
    return {{x0, y0, x1, y1}};
  }

  int x_ = std::uniform_int_distribution<std::int32_t>(xmin, xmax)(rng);
  int y_ = std::uniform_int_distribution<std::int32_t>(ymin, ymax)(rng);

  if (splitx && !splity) {
    return concat({segment(x0, y0, x_, y1, rng), segment(x_, y0, x1, y1, rng)});
  } else if (!splitx && splity) {
    return concat({segment(x0, y0, x1, y_, rng), segment(x0, y_, x1, y1, rng)});
  } else if (splitx && splity) {
    return concat({segment(x0, y0, x_, y_, rng), segment(x_, y0, x1, y_, rng),
                   segment(x0, y_, x_, y1, rng), segment(x_, y_, x1, y1, rng)});
  }
  throw std::runtime_error("unreachable");
}

struct Cell2D {
  Cell2D(int rowv, int colv) : row(rowv), col(colv) {}
  int row, col;
  Ccl::Label label{Ccl::NO_LABEL};
};

int getCellRow(const Cell2D& cell) {
  return cell.row;
}

int getCellColumn(const Cell2D& cell) {
  return cell.col;
}

Ccl::Label& getCellLabel(Cell2D& cell) {
  return cell.label;
}

bool operator==(const Cell2D& left, const Cell2D& right) {
  return left.row == right.row && left.col == right.col;
}

bool cellComp(const Cell2D& left, const Cell2D& right) {
  return (left.row == right.row) ? left.col < right.col : left.row < right.row;
}

struct Cluster2D {
  std::vector<Cell2D> cells;
  std::size_t hash{0};
};

void clusterAddCell(Cluster2D& cl, const Cell2D& cell) {
  cl.cells.push_back(cell);
}

void hash(Cluster2D& cl) {
  std::ranges::sort(cl.cells, cellComp);
  cl.hash = 0;
  for (const Cell2D& c : cl.cells) {
    boost::hash_combine(cl.hash, c.col);
  }
}

bool clHashComp(const Cluster2D& left, const Cluster2D& right) {
  return left.hash < right.hash;
}

template <typename RNG>
void genclusterw(int x, int y, int x0, int y0, int x1, int y1,
                 std::vector<Cell2D>& cells, RNG& rng, double startp = 0.5,
                 double decayp = 0.9) {
  std::vector<Cell2D> add;

  auto maybe_add = [&](int x_, int y_) {
    Cell2D c(x_, y_);
    if (std::uniform_real_distribution<double>()(rng) < startp &&
        !rangeContainsValue(cells, c)) {
      cells.push_back(c);
      add.push_back(c);
    }
  };

  // NORTH
  if (y < y1) {
    maybe_add(x, y + 1);
  }
  // NORTHEAST
  if (x < x1 && y < y1) {
    maybe_add(x + 1, y + 1);
  }
  // EAST
  if (x < x1) {
    maybe_add(x + 1, y);
  }
  // SOUTHEAST
  if (x < x1 && y > y0) {
    maybe_add(x + 1, y - 1);
  }
  // SOUTH
  if (y > y0) {
    maybe_add(x, y - 1);
  }
  // SOUTHWEST
  if (x > x0 && y > y0) {
    maybe_add(x - 1, y - 1);
  }
  // WEST
  if (x > x0) {
    maybe_add(x - 1, y);
  }
  // NORTHWEST
  if (x > x0 && y < y1) {
    maybe_add(x - 1, y + 1);
  }

  for (Cell2D& c : add) {
    genclusterw(c.row, c.col, x0, y0, x1, y1, cells, rng, startp * decayp,
                decayp);
  }
}

template <typename RNG>
Cluster2D gencluster(int x0, int y0, int x1, int y1, RNG& rng,
                     double startp = 0.5, double decayp = 0.9) {
  int x0_ = x0 + 1;
  int x1_ = x1 - 1;
  int y0_ = y0 + 1;
  int y1_ = y1 - 1;

  int x = std::uniform_int_distribution<std::int32_t>(x0_, x1_)(rng);
  int y = std::uniform_int_distribution<std::int32_t>(y0_, y1_)(rng);

  std::vector<Cell2D> cells = {Cell2D(x, y)};
  genclusterw(x, y, x0_, y0_, x1_, y1_, cells, rng, startp, decayp);

  Cluster2D cl;
  cl.cells = std::move(cells);

  return cl;
}

BOOST_AUTO_TEST_CASE(Grid_2D_rand) {
  using Cell = Cell2D;
  using CellC = std::vector<Cell>;
  using Cluster = Cluster2D;
  using ClusterC = std::vector<Cluster>;

  std::size_t sizeX = 1000;
  std::size_t sizeY = 1000;
  std::size_t startSeed = 71902647;
  std::size_t ntries = 100;

  std::cout << "Grid_2D_rand test with parameters: " << std::endl;
  std::cout << " sizeX = " << sizeX << std::endl;
  std::cout << " sizeY = " << sizeY << std::endl;
  std::cout << " startSeed = " << startSeed << std::endl;
  std::cout << " ntries = " << ntries << std::endl;

  while (ntries-- > 0) {
    std::mt19937_64 rnd(startSeed++);

    std::vector<Cluster> cls;
    std::vector<Cell> cells;
    for (Rectangle& rect : segment(0, 0, sizeX, sizeY, rnd)) {
      auto& [x0, y0, x1, y1] = rect;
      Cluster cl = gencluster(x0, y0, x1, y1, rnd);
      hash(cl);
      cells.insert(cells.end(), cl.cells.begin(), cl.cells.end());
      cls.push_back(cl);
    }

    std::shuffle(cells.begin(), cells.end(), rnd);

    ClusterC newCls = Ccl::createClusters<CellC, ClusterC>(cells);

    for (Cluster& cl : newCls) {
      hash(cl);
    }

    std::ranges::sort(cls, clHashComp);
    std::ranges::sort(newCls, clHashComp);

    BOOST_CHECK_EQUAL(cls.size(), newCls.size());
    for (std::size_t i = 0; i < cls.size(); i++) {
      BOOST_CHECK_EQUAL(cls.at(i).hash, newCls.at(i).hash);
    }
  }
}

}  // namespace Acts::Test
