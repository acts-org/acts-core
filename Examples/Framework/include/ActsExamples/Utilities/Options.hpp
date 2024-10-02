// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <array>
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <vector>

namespace ActsExamples::Options {

/// @defgroup option-types Additional types for program options
///
/// All types are intended as utility type for the user options and not as a
/// variable type for the configuration structs. They should only be used where
/// a single option can not be represented by an existing primitive types.
///
/// They also must be distinct types and can not just be typedefs; otherwise we
/// can not define the required operator{<<,>>} overloads in this namespace.
///
/// @{

/// Half open [lower,upper) interval type for a single user option.
///
/// A missing limit represents an unbounded upper or lower limit. With just
/// one defined limit the interval is just a lower/upper bound; with both
/// limits undefined, the interval is unbounded everywhere and thus contains
/// all possible values.
struct Interval {
  std::optional<double> lower;
  std::optional<double> upper;
};

/// A fixed number of real values as one user option.
///
/// @note Implemented as a subclass so it is distinct from `std::array`
///   and we can provide overloads in the same namespace.
template <std::size_t kSize>
class Reals : public std::array<double, kSize> {};

/// An arbitrary number of revaluesal  as one user option.
///
/// @note Making this a `std::vector<double>` typedef or subclass confuses
///   program options, since `std::vector<double>` is interpreted as a `double`
///   option that can be provided multiple times.
struct VariableReals {
  std::vector<double> values;
};

/// A fixed number of integers as one user option.
///
/// @note Implemented as a subclass so it is distinct from `std::array`
///   and we can provide overloads in the same namespace.
template <std::size_t kSize>
class Integers : public std::array<int, kSize> {};

/// An arbitrary number of integers as one user option.
///
/// @note Making this a `std::vector<int>` typedef or subclass confuses
///   program options, since `std::vector<int>` is interpreted as an `int`
///   option that can be provided multiple times.
struct VariableIntegers {
  std::vector<int> values;
};

/// @}

/// Extract an interval from an input of the form 'lower:upper'.
///
/// An input of the form `lower:` or `:upper` sets just one of the limits. Any
/// other input leads to an unbounded interval.
///
/// @note The more common range notation uses `lower-upper` but the `-`
///   separator complicates the parsing of negative values.
std::istream& operator>>(std::istream& is, Interval& interval);

/// Print an interval as `lower:upper`.
std::ostream& operator<<(std::ostream& os, const Interval& interval);

namespace detail {
void parseDoublesFixed(std::istream& is, std::size_t size, double* values);
void parseDoublesVariable(std::istream& is, std::vector<double>& values);
void printDoubles(std::ostream& os, std::size_t size, const double* values);
}  // namespace detail

/// Extract a fixed number of doubles from an input of the form 'x:y:z'.
///
/// @note If the values would be separated by whitespace, negative values
///   and additional command line both start with `-` and would be
///   undistinguishable.
template <std::size_t kSize>
inline std::istream& operator>>(std::istream& is, Reals<kSize>& values) {
  detail::parseDoublesFixed(is, kSize, values.data());
  return is;
}

/// Extract a variable number of doubles from an input of the form 'x:y:...'.
///
/// @note If the values would be separated by whitespace, negative values
///   and additional command line both start with `-` and would be
///   undistinguishable.
inline std::istream& operator>>(std::istream& is, VariableReals& values) {
  detail::parseDoublesVariable(is, values.values);
  return is;
}

/// Print a fixed number of doubles as `x:y:z`.
template <std::size_t kSize>
inline std::ostream& operator<<(std::ostream& os, const Reals<kSize>& values) {
  detail::printDoubles(os, kSize, values.data());
  return os;
}

/// Print a variable number of doubles as `x:y:z:...`.
inline std::ostream& operator<<(std::ostream& os, const VariableReals& values) {
  detail::printDoubles(os, values.values.size(), values.values.data());
  return os;
}

namespace detail {
void parseIntegersFixed(std::istream& is, std::size_t size, int* values);
void parseIntegersVariable(std::istream& is, std::vector<int>& values);
void printIntegers(std::ostream& os, std::size_t size, const int* values);
}  // namespace detail

/// Extract a fixed number of integers from an input of the form 'x:y:z'.
///
/// @note If the values would be separated by whitespace, negative values
///   and additional command line both start with `-` and would be
///   undistinguishable.
template <std::size_t kSize>
inline std::istream& operator>>(std::istream& is, Integers<kSize>& values) {
  detail::parseIntegersFixed(is, kSize, values.data());
  return is;
}

/// Extract a variable number of integers from an input of the form 'x:y:...'.
///
/// @note If the values would be separated by whitespace, negative values
///   and additional command line both start with `-` and would be
///   undistinguishable.
inline std::istream& operator>>(std::istream& is, VariableIntegers& values) {
  detail::parseIntegersVariable(is, values.values);
  return is;
}

/// Print a fixed number of integers as `x:y:z`.
template <std::size_t kSize>
inline std::ostream& operator<<(std::ostream& os,
                                const Integers<kSize>& values) {
  detail::printIntegers(os, kSize, values.data());
  return os;
}

/// Print a variable number of integers as `x:y:z:...`.
inline std::ostream& operator<<(std::ostream& os,
                                const VariableIntegers& values) {
  detail::printIntegers(os, values.values.size(), values.values.data());
  return os;
}

}  // namespace ActsExamples::Options
