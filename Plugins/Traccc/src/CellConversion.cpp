// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Acts include(s)
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"

// Plugin include(s)
#include "Acts/Plugins/Traccc/BarcodeMap.hpp"
#include "Acts/Plugins/Traccc/Detail/Module.hpp"

// Detray include(s)
#include "detray/core/detector.hpp"

// Traccc include(s)
#include "traccc/edm/cell.hpp"
#include "traccc/geometry/geometry.hpp"
#include "traccc/io/digitization_config.hpp"
#include "traccc/io/read_geometry.hpp"
#include "traccc/io/reader_edm.hpp"

// VecMem include(s)
#include "vecmem/memory/memory_resource.hpp"

// System include(s)
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <tuple>
#include <utility>

namespace {

/// Comparator used for sorting cells. This sorting is one of the assumptions
/// made in the clusterization algorithm
struct CellOrder {
  bool operator()(const traccc::cell& lhs, const traccc::cell& rhs) const {
    if (lhs.module_link != rhs.module_link) {
      return lhs.module_link < rhs.module_link;
    } else if (lhs.channel1 != rhs.channel1) {
      return (lhs.channel1 < rhs.channel1);
    } else {
      return (lhs.channel0 < rhs.channel0);
    }
  }
};  // struct CellOrder

}  // namespace

namespace Acts::TracccPlugin {

std::tuple<traccc::cell_collection_types::host,
           traccc::cell_module_collection_types::host>
createCellsAndModules(
    vecmem::memory_resource* mr,
    std::map<Acts::GeometryIdentifier::Value, std::vector<traccc::cell>>
        cellsMap,
    const traccc::geometry* geom, const traccc::digitization_config* dconfig,
    const std::map<Acts::GeometryIdentifier::Value, detray::geometry::barcode>*
        barcodeMap) {
  traccc::io::cell_reader_output out(mr);

  // Sort the cells.
  for (auto& [_, cells] : cellsMap) {
    std::sort(cells.begin(), cells.end(), CellOrder());
  }

  // Fill the output containers with the ordered cells and modules.
  for (const auto& [originalGeometryID, cells] : cellsMap) {
    // Modify the geometry ID of the module if a barcode map is
    // provided.
    Acts::GeometryIdentifier::Value geometryID =
        (barcodeMap != nullptr) ? barcodeMap->at(originalGeometryID).value()
                                : originalGeometryID;

    // Add the module and its cells to the output.
    out.modules.push_back(
        detail::getModule(geometryID, geom, dconfig, originalGeometryID));
    for (auto& cell : cells) {
      out.cells.push_back(cell);
      // Set the module link.
      out.cells.back().module_link = out.modules.size() - 1;
    }
  }
  return std::make_tuple(std::move(out.cells), std::move(out.modules));
}

}  // namespace Acts::TracccPlugin
