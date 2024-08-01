// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

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

// System include(s)
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <tuple>
#include <utility>

namespace Acts::TracccPlugin {

/// @brief Converts a "geometry ID -> traccc cells" map to traccc cells and modules.
/// @param mr The memory resource to use.
/// @param cellsMap A map from Acts geometry ID value to traccc cells.
/// @param geom The traccc geometry.
/// @param dconfig The traccc digitization configuration.
/// @param barcode_map A map from Acts geometry ID value to detray barcode.
/// @return A tuple containing the traccc cells (first item) and traccc modules (second item).
std::tuple<traccc::cell_collection_types::host,
           traccc::cell_module_collection_types::host>
createCellsAndModules(
    vecmem::memory_resource* mr,
    std::map<Acts::GeometryIdentifier::Value, std::vector<traccc::cell>>
        cellsMap,
    const traccc::geometry* geom, const traccc::digitization_config* dconfig,
    const std::map<Acts::GeometryIdentifier::Value, detray::geometry::barcode>*
        barcodeMap);

}  // namespace Acts::TracccPlugin
