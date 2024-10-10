// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/DetrayJsonHelper.hpp"

namespace Acts::DetrayJsonHelper {

std::tuple<unsigned int, std::vector<ActsScalar>> maskFromBounds(
    const Acts::SurfaceBounds& sBounds, bool portal) {
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

void addVolumeLink(nlohmann::json& jSurface, int vLink) {
  jSurface["volume_link"] = vLink;
}

std::size_t accelerationLink(std::span<const BinningValue> casts) {
  // Default is `brute_force`
  std::size_t accLink = 0u;
  if (casts.size() == 2u) {
    if (casts[0u] == BinningValue::binX && casts[1u] == BinningValue::binY) {
      accLink = 1u;
    } else if (casts[0u] == BinningValue::binR &&
               casts[1u] == BinningValue::binPhi) {
      accLink = 3u;
    } else if (casts[0u] == BinningValue::binZ &&
               casts[1u] == BinningValue::binPhi) {
      accLink = 4u;
    } else if (casts[0u] == BinningValue::binZ &&
               casts[1u] == BinningValue::binR) {
      accLink = 5u;
    }
  } else if (casts.size() == 3u) {
    if (casts[0u] == BinningValue::binX && casts[1u] == BinningValue::binY &&
        casts[2u] == BinningValue::binZ) {
      accLink = 2u;
    } else if (casts[0u] == BinningValue::binZ &&
               casts[1u] == BinningValue::binPhi &&
               casts[2u] == BinningValue::binR) {
      accLink = 5u;
    }
  }
  return accLink;
}
}  // namespace Acts::DetrayJsonHelper
