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
  switch (bdt) {
    case AxisBoundaryType::Open:
      os << "Open";
      break;
    case AxisBoundaryType::Bound:
      os << "Bound";
      break;
    case AxisBoundaryType::Closed:
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

Axis(ActsScalar min, ActsScalar max,
     std::size_t bins) -> Axis<AxisType::Equidistant, AxisBoundaryType::Open>;

template <AxisBoundaryType bdt>
Axis(AxisBoundaryTypeTag<bdt> /*bdt*/, ActsScalar min, ActsScalar max,
     std::size_t bins) -> Axis<AxisType::Equidistant, bdt>;

Axis(std::vector<ActsScalar> bins)
    -> Axis<AxisType::Variable, AxisBoundaryType::Open>;

template <AxisBoundaryType bdt>
Axis(AxisBoundaryTypeTag<bdt> /*bdt*/,
     std::vector<ActsScalar> bins) -> Axis<AxisType::Variable, bdt>;

}  // namespace Acts
