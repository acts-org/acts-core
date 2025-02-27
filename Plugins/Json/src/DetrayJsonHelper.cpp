// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// For whatever reason, this compilation unit does not compile
// with those assertions and GCC 13. For now just disable the
// flags in this case.
#if defined(_GLIBCXX_ASSERTIONS) && __GNUC__ == 13
#undef _GLIBCXX_ASSERTIONS
#endif

#include "Acts/Plugins/Json/DetrayJsonHelper.hpp"

namespace Acts::DetrayJsonHelper {

std::tuple<unsigned int, std::vector<double>> maskFromBounds(
    const Acts::SurfaceBounds& sBounds, bool portal) {
  SurfaceBounds::BoundsType bType = sBounds.type();
  std::vector<double> bValues = sBounds.values();
  // Return value
  unsigned int type = 13u;
  std::vector<double> boundaries = bValues;
  // Special treatment for some portals
  if (portal && bType == SurfaceBounds::BoundsType::eCylinder) {
    boundaries = {bValues.at(0u), -bValues.at(1u), bValues.at(1u)};
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
        boundaries = std::vector{0.5 * (bValues.at(2) - bValues.at(0)),
                                 0.5 * (bValues.at(3) - bValues.at(1))};
      } break;
      case SurfaceBounds::BoundsType::eCylinder: {
        boundaries =
            std::vector{bValues.at(0u), -bValues.at(1u), bValues.at(1u)};
        type = 2u;
      } break;
      case SurfaceBounds::BoundsType::eTrapezoid: {
        type = 7u;
        boundaries = std::vector{bValues.at(0u), bValues.at(1u), bValues.at(2u),
                                 1 / (2 * bValues.at(2u))};
      } break;
      case SurfaceBounds::BoundsType::eDisc: {
        boundaries = std::vector{bValues[0u], bValues[1u]};
        type = 6u;
      } break;
      default:
        break;
    }
  }
  return {type, boundaries};
}

void addVolumeLink(nlohmann::json& jSurface, int vLink) {
  jSurface["volume_link"] = vLink;
}

std::size_t accelerationLink(std::span<const AxisDirection> casts) {
  // Default is `brute_force`
  using enum AxisDirection;
  std::size_t accLink = 0u;
  if (casts.size() == 2u) {
    if (casts[0u] == AxisX && casts[1u] == AxisY) {
      accLink = 1u;
    } else if (casts[0u] == AxisR && casts[1u] == AxisPhi) {
      accLink = 3u;
    } else if (casts[0u] == AxisZ && casts[1u] == AxisPhi) {
      accLink = 4u;
    } else if (casts[0u] == AxisZ && casts[1u] == AxisR) {
      accLink = 5u;
    }
  } else if (casts.size() == 3u) {
    if (casts[0u] == AxisX && casts[1u] == AxisY && casts[2u] == AxisZ) {
      accLink = 2u;
    } else if (casts[0u] == AxisZ && casts[1u] == AxisPhi &&
               casts[2u] == AxisR) {
      accLink = 5u;
    }
  }
  return accLink;
}
}  // namespace Acts::DetrayJsonHelper
