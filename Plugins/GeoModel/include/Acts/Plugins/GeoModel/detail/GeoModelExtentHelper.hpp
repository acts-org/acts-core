// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace Acts::detail::GeoModelExentHelper {

/// @brief Helper function to find out which ones are constraint from internal / external
///
/// @param boundsEntry the bounds entry from the database
/// @param ctype the type of the constraint as string from the database
///
/// @return a vector
std::vector<AxisDirection> readBoundsConstaints(const std::string& boundsEntry,
                                                const std::string& ctype = "i");

/// @brief Helper function to find out which ones are constraint needed for binning
///
/// @param binningEntry the bounds entry from the database
///
/// @return a vector
std::vector<AxisDirection> readBinningConstraints(
    const std::vector<std::string>& binningEntry);

/// @brief Helper function to create the extent from database volume entry
///
/// @param boundsEntrySplit the bounds entry from the database
/// @param externalExtent the extend from external constraints (marked "e" in the database)
/// @param internalExtent the extend of the internal objects (marked "i" in the database)
/// @param roundInternalExtent if the bounds should be rounded to the next integer value
///
/// @return a tuple of the bounds type, the extent, and a list of binning values to be determined from the internals
std::tuple<VolumeBounds::BoundsType, Extent> extentFromTable(
    const std::vector<std::string>& boundsEntrySplit,
    const Extent& externalExtent = Extent(),
    const Extent& internalExtent = Extent(), bool roundInternalExtent = true);

}  // namespace Acts::detail::GeoModelExentHelper
