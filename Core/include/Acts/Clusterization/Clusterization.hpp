// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>
#include <vector>

namespace Acts::Ccl {

using Label = int;
constexpr Label NO_LABEL = 0;

// When looking for a cell connected to a reference cluster, the code
// always loops backward, starting from the reference cell. Since
// the cells are globally sorted column-wise, the connection function
// can therefore tell when the search should be stopped.
enum class ConnectResult {
  eNoConn,      // No connections, keep looking
  eNoConnStop,  // No connections, stop looking
  eConn         // Found connection
};

// Default connection type for 2-D grids: 4- or 8-cell connectivity
template <typename Cell>
struct Connect2D {
  bool conn8;
  Connect2D() : conn8{true} {}
  explicit Connect2D(bool commonCorner) : conn8{commonCorner} {}
  ConnectResult operator()(const Cell& ref, const Cell& iter) const;
};

// Default connection type for 1-D grids: 2-cell connectivity
template <typename Cell>
struct Connect1D {
  ConnectResult operator()(const Cell& ref, const Cell& iter) const;
};

// Default connection type based on GridDim
template <typename Cell, std::size_t GridDim = 2>
struct DefaultConnect {
  static_assert(GridDim != 1 && GridDim != 2,
                "Only grid dimensions of 1 or 2 are supported");
};

template <typename Cell>
struct DefaultConnect<Cell, 2> : public Connect2D<Cell> {
  explicit DefaultConnect(bool commonCorner) : Connect2D<Cell>(commonCorner) {}
  DefaultConnect() : DefaultConnect(true) {}
};

template <typename Cell>
struct DefaultConnect<Cell, 1> : public Connect1D<Cell> {};

/// @brief labelClusters
///
/// In-place connected component labelling using the Hoshen-Kopelman algorithm.
/// The `Cell` type must have the following functions defined:
///   int  getCellRow(const Cell&),
///   int  getCellColumn(const Cell&)
///   int& getCellLabel(Cell&)
///
/// @param [in] cells the cell collection to be labeled
/// @param [in] connect the connection type (see DefaultConnect)
template <typename CellCollection, std::size_t GridDim = 2,
          typename Connect =
              DefaultConnect<typename CellCollection::value_type, GridDim>>
void labelClusters(CellCollection& cells, Connect connect = Connect());

/// @brief mergeClusters
///
/// Merge a set of cells previously labeled (for instance with `labelClusters`)
/// into actual clusters. The Cluster type must have the following function
/// defined:
///   void clusterAddCell(Cluster&, const Cell&)
///
/// @return nothing
template <typename CellCollection, typename ClusterCollection,
          std::size_t GridDim>
ClusterCollection mergeClusters(CellCollection& /*cells*/);

/// @brief createClusters
/// Convenience function which runs both labelClusters and createClusters.
template <typename CellCollection, typename ClusterCollection,
          std::size_t GridDim = 2,
          typename Connect =
              DefaultConnect<typename CellCollection::value_type, GridDim>>
ClusterCollection createClusters(CellCollection& cells,
                                 Connect connect = Connect());

}  // namespace Acts::Ccl

#include "Acts/Clusterization/Clusterization.ipp"
