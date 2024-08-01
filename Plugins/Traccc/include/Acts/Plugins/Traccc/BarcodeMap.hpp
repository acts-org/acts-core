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

// Detray include(s)
#include "detray/core/detector.hpp"

// System include(s)
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <tuple>
#include <utility>

namespace Acts::TracccPlugin {

/// @brief Creates a map from Acts geometry ID (value) to detray barcode.
/// @param detector the detray detector.
/// @return A map (key = geometry ID value, value = detray geometry barcode).
template <typename metadata_t, typename container_t>
inline std::map<Acts::GeometryIdentifier::Value, detray::geometry::barcode>
createBarcodeMap(const detray::detector<metadata_t, container_t>& detector) {
  // Construct a map from Acts surface identifiers to Detray barcodes.
  std::map<Acts::GeometryIdentifier::Value, detray::geometry::barcode>
      barcodeMap;
  for (const auto& surface : detector.surfaces()) {
    barcodeMap[surface.source] = surface.barcode();
  }
  return barcodeMap;
}

}  // namespace Acts::TracccPlugin
