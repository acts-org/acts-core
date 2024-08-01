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

// Plugin include(s)
#include "Acts/Plugins/Traccc/DigitizationConfig.hpp"

// Traccc include(s)
#include "traccc/edm/cell.hpp"
#include "traccc/geometry/geometry.hpp"

namespace Acts::TracccPlugin::detail {

/// @brief Helper function which finds module from csv::cell in the geometry and
/// digitization config, and initializes the modules limits with the cell's
/// properties.
traccc::cell_module getModule(
    const Acts::GeometryIdentifier::Value geometryID,
    const traccc::geometry* geom, const DigitizationConfig* dconfig,
    const Acts::GeometryIdentifier::Value originalGeometryID);

}  // namespace Acts::TracccPlugin::detail
