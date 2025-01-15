// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <ostream>

namespace Acts {

/// @enum AxisDirection to specify a local axis direction
enum class AxisDirection : int {
  /// AxisX, AxisY, AxisZ are the cartesian directions in the local frame
  AxisX = 0,
  AxisY = 1,
  AxisZ = 2,
  /// AxisR is a radial direction
  AxisR = 3,
  /// AxisPhi is the azimuthal direction
  AxisPhi = 4,
  /// AxisRPhi is the radial-azimuthal direction
  AxisRPhi = 5,
  /// AxisTheta is the polar angle direction
  AxisTheta = 6,
  /// AxisEta is the pseudorapidity direction
  AxisEta = 7,
  /// AxisMag is the magnitude of the vector
  AxisMag = 8
};

/// Get all possible axis directions
/// @return a vector of all possible axis directions
const std::vector<AxisDirection>& allAxisDirections();

/// Returns the total number of axis directions
/// @return the number of axis directions
constexpr std::size_t numAxisDirections() {
  return 9;
}

/// Get an axis direction from its string name
///
/// @param name is the name of the axis direction
/// @return the axis direction
AxisDirection axisDirectionFromName(const std::string& name);

/// Get the name of a binning value as a string
/// @param aDir is the binning value
/// @return the name of the binning value
const std::string& axisDirectionName(AxisDirection aDir);

/// Output stream operator for @c AxisDirection
/// @param os is the output stream
/// @param aDir is the axis direction
/// @return the output stream
std::ostream& operator<<(std::ostream& os, AxisDirection aDir);

/// Enum which determines how the axis handle its outer boundaries
/// possible values values
enum class AxisBoundaryType {
  /// Default behaviour: out of bounds
  /// positions are filled into the over or underflow bins
  Open,
  /// Out-of-bounds positions resolve to first/last bin
  /// respectively
  Bound,
  /// Out-of-bounds positions resolve to the outermost
  /// bin on the opposite side
  Closed,
};

/// Tag helper type for Axis constructors with class template deduction
/// @tparam bdt the boundary type
template <AxisBoundaryType bdt>
struct AxisBoundaryTypeTag {};

/// Convenience typedefs for AxisBoundaryTypeTag
constexpr auto AxisOpen = AxisBoundaryTypeTag<AxisBoundaryType::Open>{};
constexpr auto AxisBound = AxisBoundaryTypeTag<AxisBoundaryType::Bound>{};
constexpr auto AxisClosed = AxisBoundaryTypeTag<AxisBoundaryType::Closed>{};

inline std::ostream& operator<<(std::ostream& os, AxisBoundaryType bdt) {
  using enum AxisBoundaryType;
  switch (bdt) {
    case Open:
      os << "Open";
      break;
    case Bound:
      os << "Bound";
      break;
    case Closed:
      os << "Closed";
      break;
  }
  return os;
}

/// Enum which determines the binning type of the axis
enum class AxisType {
  /// An axis where all bins have the same size
  Equidistant,
  /// An axis where bins can have different sizes
  Variable,
};

inline std::ostream& operator<<(std::ostream& os, AxisType type) {
  switch (type) {
    case AxisType::Equidistant:
      os << "Equidistant";
      break;
    case AxisType::Variable:
      os << "Variable";
      break;
  }
  return os;
}

/// @brief calculate bin indices from a given binning structure
///
/// This class provides some basic functionality for calculating bin indices
/// for a given binning configuration. Both equidistant as well as variable
/// binning structures are supported.
///
/// Bin intervals are defined such that the lower bound is closed and the
/// upper bound is open.
///
/// @tparam equidistant flag whether binning is equidistant (@c true)
///                     or not (@c false)
template <AxisType type, AxisBoundaryType bdt = AxisBoundaryType::Open>
class Axis;

Axis(double min, double max,
     std::size_t bins) -> Axis<AxisType::Equidistant, AxisBoundaryType::Open>;

template <AxisBoundaryType bdt>
Axis(AxisBoundaryTypeTag<bdt> /*bdt*/, double min, double max,
     std::size_t bins) -> Axis<AxisType::Equidistant, bdt>;

Axis(std::vector<double> bins)
    -> Axis<AxisType::Variable, AxisBoundaryType::Open>;

template <AxisBoundaryType bdt>
Axis(AxisBoundaryTypeTag<bdt> /*bdt*/,
     std::vector<double> bins) -> Axis<AxisType::Variable, bdt>;

}  // namespace Acts
