// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/Extent.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/ProtoAxis.hpp"

#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Acts::detail::GeoModelBinningHelper {

/// @brief Helper to transform binning string to AxisDirection enum
///
/// @param binning the binning string
inline AxisDirection toAxisDirection(const std::string& binning) {
  using enum AxisDirection;
  if (binning == "x") {
    return AxisX;
  } else if (binning == "y") {
    return AxisY;
  } else if (binning == "z") {
    return AxisZ;
  } else if (binning == "r") {
    return AxisR;
  } else if (binning == "phi") {
    return AxisPhi;
  }
  throw std::invalid_argument("GeoModelBinningHelper: Unknown binning value '" +
                              binning + "'");
}

/// @brief Convert a binning string into a ProtoiBinning description
///
/// @param binning the binning string
/// @param extent the extent of the binning
///
/// @return a ProtoAxis object
std::tuple<Acts::ProtoAxis, std::size_t> toProtoAxis(
    const std::string& binning,
    const std::optional<Extent>& extent = std::nullopt);

}  // namespace Acts::detail::GeoModelBinningHelper
