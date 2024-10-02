// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
/// - BinningValue
///   necessary access to global positions
///
enum BinningType { equidistant, arbitrary };

/// @brief flag for open/closed bins
enum BinningOption { open, closed };

/// @enum BinningValue how to take the global / local position
enum class BinningValue : int {
  binX = 0,
  binY = 1,
  binZ = 2,
  binR = 3,
  binPhi = 4,
  binRPhi = 5,
  binH = 6,
  binEta = 7,
  binMag = 8
};

/// Get all possible binning values
/// @return the binning values
const std::vector<BinningValue>& allBinningValues();

/// Returns the total number of binningvalues
/// @return the number of binning values
constexpr std::size_t numBinningValues() {
  return 9;
}

/// Get the binning value from a name
/// @param name is the name of the binning value
/// @return the binning value
BinningValue binningValueFromName(const std::string& name);

/// Get the name of a binning value as a string
/// @param bValue is the binning value
/// @return the name of the binning value
const std::string& binningValueName(BinningValue bValue);

/// Output stream operator for @c BinningValue
/// @param os is the output stream
/// @param bValue is the binning value
/// @return the output stream
std::ostream& operator<<(std::ostream& os, BinningValue bValue);

}  // namespace Acts
