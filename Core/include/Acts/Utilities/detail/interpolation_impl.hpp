// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <array>

namespace Acts::Concepts {
/// @brief check types for requirements needed by interpolation
///
/// @tparam Point1 type for specifying geometric positions
/// @tparam Point2 type for specifying geometric positions
/// @tparam Point3 type for specifying geometric positions
/// @tparam Value  type of values to be interpolated
///
/// This helper struct provides compile-time information whether the provided
/// @c Point and @c Value types can be used in the Acts::interpolate function.
///
/// The following boolean variable
/// @code{.cpp}
/// Acts::detail::can_interpolate<Point1,Point2,Point3,Value>::value
/// @endcode
///
/// is @c true if all @c Point types and @c Value fulfill the type
/// requirements for being used in the interpolation function, otherwise it is
/// @c false.
template <typename Point1, typename Point2, typename Point3, typename Value>
struct can_interpolate {
  template <typename C>
  static auto value_type_test(C* c)
      -> decltype(static_cast<C>(std::declval<double>() * std::declval<C>() +
                                 std::declval<double>() * std::declval<C>()),
                  std::true_type());
  template <typename C>
  static std::false_type value_type_test(...);

  template <typename C>
  static auto point_type_test(C* c)
      -> decltype(static_cast<double>(std::declval<C>()[0]), std::true_type());
  template <typename C>
  static std::false_type point_type_test(...);

  static const bool value =
      std::is_same_v<std::true_type,
                     decltype(value_type_test<Value>(nullptr))> &&
      std::is_same_v<std::true_type,
                     decltype(point_type_test<Point1>(nullptr))> &&
      std::is_same_v<std::true_type,
                     decltype(point_type_test<Point2>(nullptr))> &&
      std::is_same_v<std::true_type,
                     decltype(point_type_test<Point3>(nullptr))>;
};

/// @brief concept equivalent to `can_interpolate`
/// @todo this is a concept based on a traditional type trait, meaning it won't
/// have nice errors; should be rewritten as a real concept!
template <typename T, typename P1, typename P2, typename P3>
concept interpolatable = can_interpolate<P1, P2, P3, T>::value;
}  // namespace Acts::Concepts

namespace Acts::detail {

/// @brief determine number of dimension from power of 2
///
/// @tparam N power of 2
template <std::size_t N>
struct get_dimension {
  /// exponent @c d such that \f$2^d = N \f$
  static constexpr std::size_t value = get_dimension<(N >> 1)>::value + 1u;
};

/// @cond
template <>
struct get_dimension<2u> {
  static constexpr std::size_t value = 1u;
};
/// @endcond

/// @brief helper struct for performing multi-dimensional linear interpolation
///
/// @tparam T      type of values to be interpolated
/// @tparam Point1 type specifying geometric positions
/// @tparam Point2 type specifying geometric positions
/// @tparam Point3 type specifying geometric positions
/// @tparam D      current dimension on which to perform reduction
/// @tparam N      number of hyper box corners
///
/// @note
/// - Given @c U and @c V of value type @c T as well as two @c double @c a and
/// @c b, then the following must be a valid expression <tt>a * U + b * V</tt>
/// yielding an object which is (implicitly) convertible to @c T.
/// - The @c Point types must represent d-dimensional positions and support
/// coordinate access using @c operator[]. Coordinate indices must start at 0.
/// - @c N is the number of hyper box corners which is \f$2^d\f$ where \f$d\f$
/// is the dimensionality of the hyper box. The dimensionality must be
/// consistent with the provided @c Point types.
template <typename T, class Point1, class Point2, class Point3, std::size_t D,
          std::size_t N>
struct interpolate_impl;

/// @cond
// recursive implementation of linear interpolation in multiple dimensions
template <typename T, class Point1, class Point2, class Point3, std::size_t D,
          std::size_t N>
struct interpolate_impl {
  static T run(const Point1& pos, const Point2& lowerLeft,
               const Point3& upperRight, const std::array<T, N>& fields) {
    // get distance to lower boundary relative to total bin width
    const double f = (pos[D] - lowerLeft[D]) / (upperRight[D] - lowerLeft[D]);

    std::array<T, (N >> 1)> newFields{};
    for (std::size_t i = 0; i < N / 2; ++i) {
      newFields.at(i) = (1 - f) * fields.at(2 * i) + f * fields.at(2 * i + 1);
    }

    return interpolate_impl<T, Point1, Point2, Point3, D - 1, (N >> 1)>::run(
        pos, lowerLeft, upperRight, newFields);
  }
};

// simple linear interpolation in 1D
template <typename T, class Point1, class Point2, class Point3, std::size_t D>
struct interpolate_impl<T, Point1, Point2, Point3, D, 2u> {
  static T run(const Point1& pos, const Point2& lowerLeft,
               const Point3& upperRight, const std::array<T, 2u>& fields) {
    // get distance to lower boundary relative to total bin width
    const double f = (pos[D] - lowerLeft[D]) / (upperRight[D] - lowerLeft[D]);

    return (1 - f) * fields.at(0) + f * fields.at(1);
  }
};
/// @endcond
}  // namespace Acts::detail
