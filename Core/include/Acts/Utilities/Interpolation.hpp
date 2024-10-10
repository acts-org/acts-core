// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/detail/interpolation_impl.hpp"

#include <array>
#include <type_traits>

namespace Acts {

/// @brief performs linear interpolation inside a hyper box
///
/// @tparam T      type of values to be interpolated
/// @tparam N      number of hyper box corners
/// @tparam Point1 type specifying geometric positions
/// @tparam Point2 type specifying geometric positions
/// @tparam Point3 type specifying geometric positions
///
/// @param [in] position    position to which to interpolate
/// @param [in] lowerCorner generalized lower-left corner of hyper box
///                         (containing the minima of the hyper box along each
///                         dimension)
/// @param [in] upperCorner generalized upper-right corner of hyper box
///                         (containing the maxima of the hyper box along each
///                         dimension)
/// @param [in] values      field values at the hyper box corners sorted in the
///                         canonical order defined below.
///
/// @return interpolated value at given position
///
/// @pre @c position must describe a position inside the given hyper box, that
///      is \f$\text{lowerCorner}[i] \le \text{position}[i] \le
///      \text{upperCorner}[i] \quad \forall i=0, \dots, d-1\f$.
///
/// @note
/// - Given @c U and @c V of value type @c T as well as two @c double @c a and
/// @c b, then the following must be a valid expression <tt>a * U + b * V</tt>
/// yielding an object which is (implicitly) convertible to @c T.
/// - All @c Point types must represent d-dimensional positions and support
/// coordinate access using @c operator[] which should return a @c double (or a
/// value which is implicitly convertible). Coordinate indices must start at 0.
/// - @c N is the number of hyper box corners which is \f$2^d\f$ where \f$d\f$
/// is the dimensionality of the hyper box. The dimensionality must be
/// consistent with the provided @c Point types.
/// - Definition of the canonical order for sorting the field values: The hyper
/// box corners are numbered according to the following scheme. Each corner is
/// defined by the set of lower/upper boundary limits in each dimension @c i.
/// This can be represented by a binary code (from left to right) where a @c 0
/// stands for a lower bound along this axis and a @c 1 stand for the upper
/// bound along this axis. The left most bit corresponds to the first dimension
/// and the bits to the left correspond to the 2nd, 3rd... dimension. The binary
/// code can be interpreted as integer which gives the number of the
/// corresponding hyper box corner. The field values are ordered according to
/// ascending hyper box corner numbers.<br />
/// As an example assume we have a 3D box with @c lowerCorner = (1,2,3) and @c
/// upperCorner = (4,5,6). The eight corners with their bit patterns and corner
/// numbers are:
///    - (1,2,3): 000 = 0
///    - (1,2,6): 001 = 1
///    - (1,5,3): 010 = 2
///    - (1,5,6): 011 = 3
///    - (4,2,3): 100 = 4
///    - (4,2,6): 101 = 5
///    - (4,5,3): 110 = 6
///    - (4,5,6): 111 = 7
template <typename T, std::size_t N, class Point1, class Point2 = Point1,
          class Point3 = Point2>
inline T interpolate(const Point1& position, const Point2& lowerCorner,
                     const Point3& upperCorner, const std::array<T, N>& values)
  requires(Concepts::interpolatable<T, Point1, Point2, Point3>)
{
  return detail::interpolate_impl<T, Point1, Point2, Point3,
                                  detail::get_dimension<N>::value - 1,
                                  N>::run(position, lowerCorner, upperCorner,
                                          values);
}

}  // namespace Acts
