// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Range1D.hpp"

#include <algorithm>
#include <limits>

namespace Acts {
/// @brief An orthogonal range in an arbitrary number of dimensions
///
/// By combining a number one-dimensional ranges we can (under the assumption
/// that our axes are orthogonal) construct an orthogonal range of values. In
/// other words, a hyperrectangular volume in space.
///
/// @tparam Dims The number of dimensions in our range
/// @tparam Type The scalar type of our ranges
/// @tparam Vector The vector type used to define coordinates
template <std::size_t Dims, typename Type,
          template <typename, std::size_t> typename Vector = std::array>
class RangeXD {
 public:
  /// @brief The type used to describe coordinates in our range
  using coordinate_t = Vector<Type, Dims>;

  /// @brief Determine whether this range is degenerate
  ///
  /// A degenerate multi-dimensional range has no volume and cannot contain any
  /// values. This is the case if any of its dimensions are degenerate.
  ///
  /// @return true The range is degenerate
  /// @return false The range is not degenerate
  bool degenerate(void) const {
    for (std::size_t i = 0; i < Dims; ++i) {
      if (m_dims[i].degenerate()) {
        return true;
      }
    }
    return false;
  }

  /// @brief Determine whether the range contains a certain point
  ///
  /// This is true if and only if the range contains the point in all of its
  /// dimensions.
  ///
  /// @param v The coordinate to check for membership in the range
  ///
  /// @return true The coordinate is inside the range
  /// @return false The coordinate is outside the range
  bool contains(const coordinate_t& v) const {
    for (std::size_t i = 0; i < Dims; ++i) {
      if (!m_dims[i].contains(v[i])) {
        return false;
      }
    }

    return true;
  }

  /// @brief Access one of the dimensional ranges of the volume
  ///
  /// @param i The index of the dimension to access
  /// @return A reference to the dimension contained in this range
  Range1D<Type>& operator[](const std::size_t& i) { return m_dims[i]; }

  /// @brief Access one of the dimensional ranges of the volume
  ///
  /// @param i The index of the dimension to access
  /// @return A reference to the dimension contained in this range
  const Range1D<Type>& operator[](const std::size_t& i) const {
    return m_dims[i];
  }

  /// @brief Determine whether two ranges are equal
  ///
  /// Two n-dimensional ranges are equal if and only if they are equal in each
  /// of their n dimensions.
  ///
  /// @param o The other range to check for equality
  ///
  /// @return true The ranges are equal
  /// @return false The ranges are not equal
  bool operator==(const RangeXD<Dims, Type, Vector>& o) const {
    for (std::size_t i = 0; i < Dims; ++i) {
      if (!(this->m_dims[i] == o[i])) {
        return false;
      }
    }

    return true;
  }

  /// @brief Determine whether one range is a subset of another range
  ///
  /// One range is a subset of another range if and only if all points
  /// contained within the first set are also contained within the second set.
  /// Alternatively, this is equivalent to each of the first range's
  /// one-dimensional ranges being a subset of the second range's equivalent
  /// one-dimensional range.
  ///
  /// @param o The other range to compare to
  ///
  /// @return true The first range is a subset of the second range
  /// @return false The first range is not a subset of the second range
  bool operator<=(const RangeXD<Dims, Type, Vector>& o) const {
    for (std::size_t i = 0; i < Dims; ++i) {
      if (!(this->m_dims[i] <= o[i])) {
        return false;
      }
    }

    return true;
  }

  /// @brief Determine whether one range is a superset of another range
  ///
  /// One range is a superset of another range if and only if all points
  /// contained within the second range are also contained within the first
  /// range. Alternatively, this is equivalent to each of the one-dimensional
  /// ranges in the first range being a superset of the corresponding
  /// one-dimensional range in the second range.
  ///
  /// @param o The other range to compare to
  ///
  /// @return true The left-hand range is a superset of the right-hand range
  /// @return false The left-hand range is not a superset of the right-hand
  /// range
  bool operator>=(const RangeXD<Dims, Type, Vector>& o) const {
    for (std::size_t i = 0; i < Dims; ++i) {
      if (!(this->m_dims[i] >= o[i])) {
        return false;
      }
    }

    return true;
  }

  /// @brief Compute the intersection of this range with another range
  ///
  /// The intersection of one orthogonal range with another orthogonal range is
  /// in itself an orthogonal range. This operation is commutative. This
  /// intersection between two n-dimensional ranges is defined simply as the
  /// intersection in each dimension of the two ranges.
  ///
  /// @param o The orthogonal range to compute the intersection with
  ///
  /// @return The intersection between the ranges
  RangeXD<Dims, Type, Vector> operator&(
      const RangeXD<Dims, Type, Vector>& o) const {
    RangeXD<Dims, Type> res;

    for (std::size_t i = 0; i < Dims; ++i) {
      res[i] = m_dims[i] & o[i];
    }

    return res;
  }

  /// @brief Update the range to the intersection with another range
  ///
  /// This is the assignment version of the operator& method, meaning that it
  /// updates the object on which it is called rather than producing a new
  /// range.
  ///
  /// @param o The range to compute the intersection with
  ///
  /// @return This object
  RangeXD<Dims, Type, Vector>& operator&=(
      const RangeXD<Dims, Type, Vector>& o) {
    for (std::size_t i = 0; i < Dims; ++i) {
      m_dims[i] &= o[i];
    }

    return *this;
  }

  /// @brief Determine whether this range intersects another
  ///
  /// Two n-dimensional ranges intersect if and only if they intersect in every
  /// one of their n dimensions. Otherwise, they are disjoint.
  ///
  /// @param r The other range to check
  ///
  /// @return true The ranges intersect
  /// @return false The ranges do not intersect
  bool operator&&(const RangeXD<Dims, Type, Vector>& r) const {
    for (std::size_t i = 0; i < Dims; ++i) {
      if (!(m_dims[i] && r[i])) {
        return false;
      }
    }

    return true;
  }

  /// @brief Represent the range as a string
  ///
  /// This method produces a helpful string that can be used to debug the range
  /// if needed. Not really designed to be used in production code.
  ///
  /// @return A string representing the range
  std::string toString(void) const {
    std::stringstream s;

    for (std::size_t i = 0; i < Dims; ++i) {
      s << m_dims[i].min() << " <= v[" << i << "] <= " << m_dims[i].max();
      if (i != Dims - 1) {
        s << ", ";
      }
    }

    return s.str();
  }

 private:
  std::array<Range1D<Type>, Dims> m_dims;
};
}  // namespace Acts
