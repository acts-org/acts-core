// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <array>
#include <vector>

#include <boost/pending/disjoint_sets.hpp>

namespace Acts::Ccl::internal {

template <typename Cell, std::size_t GridDim>
struct Compare {
  static_assert(GridDim != 1 && GridDim != 2,
                "Only grid dimensions of 1 or 2 are supported");
};

// Comparator function object for cells, column-wise ordering
// Specialization for 1-D grids
template <Acts::Ccl::HasRetrievableColumnInfo Cell>
struct Compare<Cell, 1> {
  bool operator()(const Cell& c0, const Cell& c1) const {
    int col0 = getCellColumn(c0);
    int col1 = getCellColumn(c1);
    return col0 < col1;
  }
};

// Specialization for 2-D grid
template <typename Cell>
  requires(Acts::Ccl::HasRetrievableColumnInfo<Cell> &&
           Acts::Ccl::HasRetrievableRowInfo<Cell>)
struct Compare<Cell, 2> {
  bool operator()(const Cell& c0, const Cell& c1) const {
    int row0 = getCellRow(c0);
    int row1 = getCellRow(c1);
    int col0 = getCellColumn(c0);
    int col1 = getCellColumn(c1);
    return (col0 == col1) ? row0 < row1 : col0 < col1;
  }
};

// Simple wrapper around boost::disjoint_sets. In theory, could use
// boost::vector_property_map and use boost::disjoint_sets without
// wrapping, but it's way slower
class DisjointSets {
 public:
  explicit DisjointSets(std::size_t initial_size = 128)
      : m_size(initial_size),
        m_rank(m_size),
        m_parent(m_size),
        m_ds(&m_rank[0], &m_parent[0]) {}

  Label makeSet() {
    // Empirically, m_size = 128 seems to be good default. If we
    // exceed this, take a performance hit and do the right thing.
    while (m_globalId >= m_size) {
      m_size *= 2;
      m_rank.resize(m_size);
      m_parent.resize(m_size);
      m_ds = boost::disjoint_sets<std::size_t*, std::size_t*>(&m_rank[0],
                                                              &m_parent[0]);
    }
    m_ds.make_set(m_globalId);
    return static_cast<Label>(m_globalId++);
  }

  void unionSet(std::size_t x, std::size_t y) { m_ds.union_set(x, y); }
  Label findSet(std::size_t x) { return static_cast<Label>(m_ds.find_set(x)); }

 private:
  std::size_t m_globalId = 1;
  std::size_t m_size;
  std::vector<std::size_t> m_rank;
  std::vector<std::size_t> m_parent;
  boost::disjoint_sets<std::size_t*, std::size_t*> m_ds;
};

template <std::size_t BufSize>
struct ConnectionsBase {
  std::size_t nconn{0};
  std::array<Label, BufSize> buf;
  ConnectionsBase() { std::fill(buf.begin(), buf.end(), NO_LABEL); }
};

template <std::size_t GridDim>
class Connections {};

// On 1-D grid, cells have 1 backward neighbor
template <>
struct Connections<1> : public ConnectionsBase<1> {
  using ConnectionsBase::ConnectionsBase;
};

// On a 2-D grid, cells have 4 backward neighbors
template <>
struct Connections<2> : public ConnectionsBase<4> {
  using ConnectionsBase::ConnectionsBase;
};

// Cell collection logic
template <typename Cell, typename Connect, std::size_t GridDim>
Connections<GridDim> getConnections(typename std::vector<Cell>::iterator it,
                                    std::vector<Cell>& set, Connect connect) {
  Connections<GridDim> seen;
  typename std::vector<Cell>::iterator it_2{it};

  while (it_2 != set.begin()) {
    it_2 = std::prev(it_2);

    ConnectResult cr = connect(*it, *it_2);
    if (cr == ConnectResult::eNoConnStop) {
      break;
    }
    if (cr == ConnectResult::eNoConn) {
      continue;
    }
    if (cr == ConnectResult::eConn) {
      seen.buf[seen.nconn] = getCellLabel(*it_2);
      seen.nconn += 1;
      if (seen.nconn == seen.buf.size()) {
        break;
      }
    }
  }
  return seen;
}

template <typename CellCollection, typename ClusterCollection>
  requires(
      Acts::Ccl::HasRetrievableLabelInfo<typename CellCollection::value_type> &&
      Acts::Ccl::CanAcceptCell<typename CellCollection::value_type,
                               typename ClusterCollection::value_type>)
ClusterCollection mergeClustersImpl(CellCollection& cells) {
  using Cluster = typename ClusterCollection::value_type;

  if (cells.empty()) {
    return {};
  }

  // Accumulate clusters into the output collection
  ClusterCollection outv;
  Cluster cl;
  int lbl = getCellLabel(cells.front());
  for (auto& cell : cells) {
    if (getCellLabel(cell) != lbl) {
      // New cluster, save previous one
      outv.push_back(std::move(cl));
      cl = Cluster();
      lbl = getCellLabel(cell);
    }
    clusterAddCell(cl, cell);
  }
  // Get the last cluster as well
  outv.push_back(std::move(cl));

  return outv;
}

}  // namespace Acts::Ccl::internal

namespace Acts::Ccl {

template <typename Cell>
  requires(Acts::Ccl::HasRetrievableColumnInfo<Cell> &&
           Acts::Ccl::HasRetrievableRowInfo<Cell>)
ConnectResult Connect2D<Cell>::operator()(const Cell& ref,
                                          const Cell& iter) const {
  int deltaRow = std::abs(getCellRow(ref) - getCellRow(iter));
  int deltaCol = std::abs(getCellColumn(ref) - getCellColumn(iter));
  // Iteration is column-wise, so if too far in column, can
  // safely stop
  if (deltaCol > 1) {
    return ConnectResult::eNoConnStop;
  }
  // For same reason, if too far in row we know the pixel is not
  // connected, but need to keep iterating
  if (deltaRow > 1) {
    return ConnectResult::eNoConn;
  }
  // Decide whether or not cluster is connected based on 4- or
  // 8-connectivity
  if ((deltaRow + deltaCol) <= (conn8 ? 2 : 1)) {
    return ConnectResult::eConn;
  }
  return ConnectResult::eNoConn;
}

template <Acts::Ccl::HasRetrievableColumnInfo Cell>
ConnectResult Connect1D<Cell>::operator()(const Cell& ref,
                                          const Cell& iter) const {
  int deltaCol = std::abs(getCellColumn(ref) - getCellColumn(iter));
  return deltaCol == 1 ? ConnectResult::eConn : ConnectResult::eNoConnStop;
}

template <std::size_t GridDim>
void recordEquivalences(const internal::Connections<GridDim> seen,
                        internal::DisjointSets& ds) {
  // Sanity check: first element should always have
  // label if nconn > 0
  if (seen.nconn > 0 && seen.buf[0] == NO_LABEL) {
    throw std::logic_error("seen.nconn > 0 but seen.buf[0] == NO_LABEL");
  }
  for (std::size_t i = 1; i < seen.nconn; i++) {
    // Sanity check: since connection lookup is always backward
    // while iteration is forward, all connected cells found here
    // should have a label
    if (seen.buf[i] == NO_LABEL) {
      throw std::logic_error("i < seen.nconn but see.buf[i] == NO_LABEL");
    }
    // Only record equivalence if needed
    if (seen.buf[0] != seen.buf[i]) {
      ds.unionSet(seen.buf[0], seen.buf[i]);
    }
  }
}

template <typename CellCollection, std::size_t GridDim, typename Connect>
  requires(
      Acts::Ccl::HasRetrievableLabelInfo<typename CellCollection::value_type>)
void labelClusters(CellCollection& cells, Connect connect) {
  using Cell = typename CellCollection::value_type;

  internal::DisjointSets ds{};

  // Sort cells by position to enable in-order scan
  std::ranges::sort(cells, internal::Compare<Cell, GridDim>());

  // First pass: Allocate labels and record equivalences
  for (auto it = std::ranges::begin(cells); it != std::ranges::end(cells);
       ++it) {
    const internal::Connections<GridDim> seen =
        internal::getConnections<Cell, Connect, GridDim>(it, cells, connect);
    if (seen.nconn == 0) {
      // Allocate new label
      getCellLabel(*it) = ds.makeSet();
    } else {
      recordEquivalences(seen, ds);
      // Set label for current cell
      getCellLabel(*it) = seen.buf[0];
    }
  }

  // Second pass: Merge labels based on recorded equivalences
  for (auto& cell : cells) {
    Label& lbl = getCellLabel(cell);
    lbl = ds.findSet(lbl);
  }
}

template <typename CellCollection, typename ClusterCollection,
          std::size_t GridDim = 2>
  requires(GridDim == 1 || GridDim == 2) &&
          Acts::Ccl::HasRetrievableLabelInfo<
              typename CellCollection::value_type>
ClusterCollection mergeClusters(CellCollection& cells) {
  using Cell = typename CellCollection::value_type;
  if constexpr (GridDim > 1) {
    // Sort the cells by their cluster label, only needed if more than
    // one spatial dimension
    std::ranges::sort(cells, {}, [](Cell& c) { return getCellLabel(c); });
  }

  return internal::mergeClustersImpl<CellCollection, ClusterCollection>(cells);
}

template <typename CellCollection, typename ClusterCollection,
          std::size_t GridDim, typename Connect>
ClusterCollection createClusters(CellCollection& cells, Connect connect) {
  labelClusters<CellCollection, GridDim, Connect>(cells, connect);
  return mergeClusters<CellCollection, ClusterCollection, GridDim>(cells);
}

}  // namespace Acts::Ccl
