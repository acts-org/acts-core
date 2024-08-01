// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Acts include(s)
#include "Acts/Geometry/GeometryIdentifier.hpp"

// Acts Examples include(s)
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/Traccc/Common/Conversion/CellMapConversion.hpp"

// Traccc include(s)
#include "traccc/edm/cell.hpp"

// System include(s).
#include <cstdint>
#include <cstdlib>
#include <map>
#include <vector>

namespace ActsExamples::Traccc::Common::Conversion {

/// @brief Gets the time of the cell.
/// @note Currently, it always returns 0.
inline float getTime(const Cluster::Cell& /*cell*/) {
  return 0.f;
}

/// @brief Gets the activation of the cell.
inline float getActivation(const Cluster::Cell& cell) {
  return static_cast<float>(cell.activation);
}

/// @brief Gets the row of the cell.
inline unsigned int getRow(const Cluster::Cell& cell) {
  if (cell.bin[0] > UINT_MAX) {
    throw std::runtime_error(
        "Overflow will occur when casting to unsigned int.");
  }
  return static_cast<unsigned int>(cell.bin[0]);
}

/// @brief Gets the column of the cell.
inline unsigned int getColumn(const Cluster::Cell& cell) {
  if (cell.bin[0] > UINT_MAX) {
    throw std::runtime_error(
        "Overflow will occur when casting to unsigned int.");
  }
  return static_cast<unsigned int>(cell.bin[1]);
}

/// @brief Creates a traccc cell from a generic cell type.
/// @param cell the cell.
/// @param moduleLink the module link value to set for the traccc cell that is created.
/// @returns a traccc cell.
/// @note the functions getRow(cell_t), getColumn(cell_t), getActivation(cell_t), getTime(cell_t) are expected.
inline traccc::cell tracccCell(const Cluster::Cell& cell,
                               const traccc::cell::link_type moduleLink = 0) {
  return traccc::cell{getRow(cell), getColumn(cell), getActivation(cell),
                      getTime(cell), moduleLink};
}

/// @brief Converts a "geometry ID -> generic cell collection type" map to a "geometry ID -> traccc cell collection" map.
/// @note The function sets the module link of the cells in the output to 0.
/// @return Map from geometry ID to its cell data (as a vector of traccc cell data)
std::map<std::uint64_t, std::vector<traccc::cell>> tracccCellsMap(
    const std::map<Acts::GeometryIdentifier, std::vector<Cluster::Cell>>& map) {
  std::map<std::uint64_t, std::vector<traccc::cell>> tracccCellMap;
  for (const auto& [geometryID, cells] : map) {
    std::vector<traccc::cell> tracccCells;
    for (const auto& cell : cells) {
      tracccCells.push_back(tracccCell(cell));
    }
    tracccCellMap.insert({geometryID.value(), std::move(tracccCells)});
  }
  return tracccCellMap;
}

}  // namespace ActsExamples::Traccc::Common::Conversion
