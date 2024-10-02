// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <span>
#include <tuple>

#include <nlohmann/json.hpp>

namespace Acts::DetrayJsonHelper {

/// @brief Helper function to switch keys from ACTS to detray
///
/// DETRAY types @todo change to detray imports when available
///    annulus2 = 0u,
///    cuboid3 = 1u,
///    cylinder2 = 2u,
///    cylinder3 = 3u,
///    portal_cylinder2 = 4u,
///    rectangle2 = 5u,
///    ring2 = 6u,
///    trapezoid2 = 7u,
///    cell_wire = 8u,
///    straw_wire = 9u,
///    single1 = 10u,
///    single2 = 11u,
///    single3 = 12u,
///    unknown = 13u
///
/// @param sBounds is the surface bounds type
/// @param portal is the flag for conversion into detray portal format
///
/// @return type and value array in detray format
std::tuple<unsigned int, std::vector<ActsScalar>> maskFromBounds(
    const Acts::SurfaceBounds& sBounds, bool portal = false);

/// @brief add volume link
///
/// @param jSurface [in,out] is the json object to be patched
/// @param vLink is the volume link to be added
void addVolumeLink(nlohmann::json& jSurface, int vLink);

/// Determine the acceleration link from a grid
///
///
///   brute_force = 0u,      // try all
///   cartesian2_grid = 1u,  // rectangle, trapezoid, (triangle) grids
///   cuboid3_grid = 2u,     // cuboid grid
///   polar2_grid = 3u,      // ring/disc, annulus grids
///   cylinder2_grid = 4u,   // 2D cylinder grid
///   cylinder3_grid = 5u,   // 3D cylinder grid
///
/// @param casts are the grid axes cast types
///
/// @return the acceleration link idnetifier
std::size_t accelerationLink(std::span<const BinningValue> casts);

}  // namespace Acts::DetrayJsonHelper
