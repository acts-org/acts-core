// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <string>
#include <type_traits>
#include <vector>

namespace Acts {

/// @enum BinningType, BinningOption & BinningAccess
///
///- BinningType:
///
///  Enumeration to qualify the binning type for the use of the
///  LayerArrayCreator and the TrackingVolumeArrayCreator
///
/// - BinningOption:
///   open:   [0,max]
///   closed:  0 -> nextbin -> max -> 0
///
/// - AxisDirection
///   necessary access to global positions
///
enum BinningType { equidistant, arbitrary };

/// @brief flag for open/closed bins
enum BinningOption { open, closed };

}  // namespace Acts
