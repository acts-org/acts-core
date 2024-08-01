// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <string>
#include <tuple>

#include <nlohmann/json.hpp>

#include "detray/io/frontend/definitions.hpp"

namespace Acts::DetrayConversionHelper {

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
inline static std::tuple<unsigned int, std::vector<ActsScalar>> maskFromBounds(
    const Acts::SurfaceBounds& sBounds, bool portal = false) {
  auto bType = sBounds.type();
  auto bValues = sBounds.values();
  // Return value
  unsigned int type = 13u;
  std::vector<double> boundaries = bValues;
  // Special treatment for some portals
  if (portal && bType == SurfaceBounds::BoundsType::eCylinder) {
    boundaries = {bValues[0u], -bValues[1u], bValues[1u]};
    type = 4u;
  } else {
    switch (bType) {
      case SurfaceBounds::BoundsType::eAnnulus: {
        type = 0u;
      } break;
      case SurfaceBounds::BoundsType::eRectangle: {
        type = 5u;
        // ACTS: eMinX = 0, eMinY = 1, eMaxX = 2, eMaxY = 3,
        // detray: e_half_x, e_half_y
        boundaries = {0.5 * (bValues[2] - bValues[0]),
                      0.5 * (bValues[3] - bValues[1])};
      } break;
      case SurfaceBounds::BoundsType::eCylinder: {
        boundaries = {bValues[0u], -bValues[1u], bValues[1u]};
        type = 2u;
      } break;
      case SurfaceBounds::BoundsType::eTrapezoid: {
        type = 7u;
        boundaries = {bValues[0u], bValues[1u], bValues[2u],
                      1 / (2 * bValues[2u])};
      } break;
      case SurfaceBounds::BoundsType::eDisc: {
        boundaries = {bValues[0u], bValues[1u]};
        type = 6u;
      } break;
      default:
        break;
    }
  }
  return std::tie(type, boundaries);
}

/// Determine the acceleration link from a grid
///
/// @param casts are the grid axes cast types
///
/// @return the acceleration link idnetifier
template <typename binning_values_t>
inline static detray::io::accel_id accelerationLink(
    const binning_values_t& casts) {
  // Default is `brute_force`
  detray::io::accel_id accLink = detray::io::accel_id::brute_force;
  if (casts.size() == 2u) {
    if (casts[0u] == Acts::BinningValue::binX &&
        casts[1u] == Acts::BinningValue::binY) {
      accLink = detray::io::accel_id::cartesian2_grid;
    } else if (casts[0u] == Acts::BinningValue::binR &&
               casts[1u] == Acts::BinningValue::binPhi) {
      accLink = detray::io::accel_id::polar2_grid;
    } else if (casts[0u] == Acts::BinningValue::binZ &&
               casts[1u] == Acts::BinningValue::binPhi) {
      accLink = detray::io::accel_id::cylinder2_grid;
    } else if (casts[0u] == Acts::BinningValue::binZ &&
               casts[1u] == Acts::BinningValue::binR) {
      accLink = detray::io::accel_id::cylinder3_grid;
    }
  } else if (casts.size() == 3u) {
    if (casts[0u] == Acts::BinningValue::binX &&
        casts[1u] == Acts::BinningValue::binY &&
        casts[2u] == Acts::BinningValue::binZ) {
      accLink = detray::io::accel_id::cuboid3_grid;
    } else if (casts[0u] == Acts::BinningValue::binZ &&
               casts[1u] == Acts::BinningValue::binPhi &&
               casts[2u] == Acts::BinningValue::binR) {
      accLink = detray::io::accel_id::cylinder3_grid;
    }
  }
  return accLink;
}

}  // namespace Acts::DetrayConversionHelper
