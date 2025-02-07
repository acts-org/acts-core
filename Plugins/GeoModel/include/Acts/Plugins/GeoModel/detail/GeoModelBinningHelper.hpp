// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <exception>
#include <optional>
#include <string>
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
/// @return a ProtoBinning object
Acts::Experimental::ProtoBinning toProtoBinning(
    const std::string& binning,
    const std::optional<Extent>& extent = std::nullopt);

}  // namespace Acts::detail::GeoModelBinningHelper
