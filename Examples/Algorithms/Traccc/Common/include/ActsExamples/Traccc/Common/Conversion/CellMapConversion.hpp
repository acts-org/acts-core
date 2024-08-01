// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Acts include(s)
#include "Acts/Geometry/GeometryIdentifier.hpp"

// Acts Examples include(s)
#include "ActsExamples/EventData/Cluster.hpp"

// Traccc include(s)
#include "traccc/edm/cell.hpp"

// System include(s).
#include <cstdint>
#include <cstdlib>
#include <map>
#include <vector>

namespace ActsExamples::Traccc::Common::Conversion {

/// @brief Converts a "geometry ID -> generic cell collection type" map to a "geometry ID -> traccc cell collection" map.
/// @note The function sets the module link of the cells in the output to 0.
/// @return Map from geometry ID to its cell data (as a vector of traccc cell data)
std::map<std::uint64_t, std::vector<traccc::cell>> tracccCellsMap(
    const std::map<Acts::GeometryIdentifier, std::vector<Cluster::Cell>>& map);

}  // namespace ActsExamples::Traccc::Common::Conversion
