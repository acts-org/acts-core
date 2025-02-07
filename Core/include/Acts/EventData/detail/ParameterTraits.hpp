// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace Acts::detail {

/// Traits class for an unrestricted parameter.
struct UnrestrictedParameterTraits {
  /// Values need no adjustment.
  static constexpr bool may_modify_value = false;

  /// Get the corrected value within the limits. This is a no-op here.
  template <typename value_t>
  static constexpr const value_t& getValue(const value_t& value) {
    return value;
  }
  /// Compute the difference between two values.
  template <typename value_t>
  static constexpr value_t getDifference(const value_t& lhs,
                                         const value_t& rhs) {
    return lhs - rhs;
  }
};

/// Traits class for a parameter with a restricted value range.
///
/// @tparam limits_t a type with static `lowest()` and `max()` member functions
///
/// This parameter type could be useful to describe parameter with physical
/// meaningful bounds (e.g. radius).
template <typename limits_t>
struct RestrictedParameterTraits {
  /// Parameter values may need adjustment.
  static constexpr bool may_modify_value = true;
  /// Lower bound of range.
  static constexpr double min = limits_t::lowest();
  /// Upper bound of range.
  static constexpr double max = limits_t::max();

  /// Get the corrected value within the limits.
  template <typename value_t>
  static constexpr value_t getValue(const value_t& value) {
    return std::clamp(value, static_cast<value_t>(min),
                      static_cast<value_t>(max));
  }
  /// Compute the difference between two values with limit handling.
  template <typename value_t>
  static constexpr value_t getDifference(const value_t& lhs,
                                         const value_t& rhs) {
    return getValue(lhs) - getValue(rhs);
  }
};

/// Traits class for a parameter with a cyclic value range.
///
/// @tparam limits_t a type with static `lowest()` and `max()` member functions
///
/// This parameter type is useful to e.g. describe angles.
template <typename limits_t>
struct CyclicParameterTraits {
  /// Parameter values may need adjustment.
  static constexpr bool may_modify_value = true;
  /// Lower bound of range.
  static constexpr double min = limits_t::lowest();
  /// Upper bound of range.
  static constexpr double max = limits_t::max();

  /// Get the corrected value folded into the central range.
  template <typename value_t>
  static constexpr value_t getValue(const value_t& value) {
    if ((min <= value) && (value < max)) {
      return value;
    } else {
      return value - (max - min) * std::floor((value - min) / (max - min));
    }
  }
  /// Compute the smallest equivalent difference when using the periodicity.
  template <typename value_t>
  static constexpr value_t getDifference(const value_t& lhs,
                                         const value_t& rhs) {
    constexpr value_t range = (max - min);
    value_t delta = getValue(lhs) - getValue(rhs);
    if ((2 * delta) < -range) {
      return delta + range;
    } else if (range < (2 * delta)) {
      return delta - range;
    } else {
      return delta;
    }
  }
};

// Limit types for parameter traits.
//
// The functions names are chosen to be consistent w/ std::numeric_limits
struct PhiBoundParameterLimits {
  static constexpr double lowest() { return -std::numbers::pi; }
  static constexpr double max() { return std::numbers::pi; }
};
struct ThetaBoundParameterLimits {
  static constexpr double lowest() { return 0; }
  static constexpr double max() { return std::numbers::pi; }
};

// Traits implementation structs for single parameters.
//
// Separate implementation structs are needed to generate a compile-time error
// in case of an unsupported enum type/ index combination.
template <typename index_t, index_t kIndex>
struct ParameterTraitsImpl;
template <>
struct ParameterTraitsImpl<BoundIndices, BoundIndices::eBoundPhi> {
  using Type = CyclicParameterTraits<PhiBoundParameterLimits>;
};
template <>
struct ParameterTraitsImpl<BoundIndices, BoundIndices::eBoundTheta> {
  using Type = RestrictedParameterTraits<ThetaBoundParameterLimits>;
};
template <BoundIndices kIndex>
struct ParameterTraitsImpl<BoundIndices, kIndex> {
  // other bound parameters not explicitly specified above are unrestricted
  using Type = UnrestrictedParameterTraits;
};
template <FreeIndices kIndex>
struct ParameterTraitsImpl<FreeIndices, kIndex> {
  // all free parameters components are unrestricted
  using Type = UnrestrictedParameterTraits;
};

/// Parameter traits for one specific parameter in one of the indices enums.
///
/// @tparam index_t Parameter indices enum
/// @tparam kIndex Enum index value to identify a parameter
///
/// This type resolves directly to one of the parameter traits classes defined
/// above and allows for uniform access.
template <typename index_t, index_t kIndex>
using ParameterTraits = typename ParameterTraitsImpl<index_t, kIndex>::Type;

// Traits implementation structs for all parameters in an indices enum.
//
// Separate implementation structs are needed to generate a compile-time error
// in case of an unsupported indices enum. Also required since template
// variables can not have an undefined default case.
template <typename indices_t>
struct ParametersTraitsImpl;
template <>
struct ParametersTraitsImpl<BoundIndices> {
  static constexpr std::size_t kSize =
      static_cast<std::size_t>(BoundIndices::eBoundSize);
};
template <>
struct ParametersTraitsImpl<FreeIndices> {
  static constexpr std::size_t kSize =
      static_cast<std::size_t>(FreeIndices::eFreeSize);
};

/// The maximum parameters vector size definable for an indices enum.
template <typename indices_t>
constexpr std::size_t kParametersSize = ParametersTraitsImpl<indices_t>::kSize;

}  // namespace Acts::detail
