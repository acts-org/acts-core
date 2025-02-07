// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/GeoModel/GeoModelTree.hpp"

#include <memory>
#include <string>

namespace Acts::GeoModelReader {

/// @brief Read the GeoModel from the database
///
/// @param dbPath path to the database
///
/// @return world/top volume of the GeoModel tree in memory
GeoModelTree readFromDb(const std::string& dbPath);

}  // namespace Acts::GeoModelReader
