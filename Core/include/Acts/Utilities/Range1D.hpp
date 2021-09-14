// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <algorithm>
#include <limits>

namespace Acts {
/// @brief A one-dimensional range between two points.
///
/// This type describes a one-demensional range of values, designed to be used
/// in the construction of more complex multi-dimensional types. The class
/// provides functionality for growing and shrinking the range, as well as some
/// other utilities. These ranges are half-open, including the minimum but
/// excluding the maximum.
///
/// @tparam Type The scalar type of the values contained in this range.
template <typename Type>
class Range1D {
 public:
  /// @brief Construct a new degenerate range object
  ///
  /// This constructor coonstructs a degenerate range object with a maximum
  /// lower than the minimum. In other words, this range is empty.
  Range1D()
      : m_min(std::numeric_limits<Type>::lowest()),
        m_max(std::numeric_limits<Type>::max()) {}

  /// @brief Construct a new range object from a lower and upper bound
  ///
  /// Construct a new range object given the values for the minimum and
  /// maximum. Note that it is perfectly possible to construct a degenerate
  /// range in this way.
  ///
  /// @param min The minimum value in the range (inclusive)
  /// @param max The maximum value in the range (exclusive)
  Range1D(Type min, Type max) : m_min(min), m_max(max) {}

  /// @brief Construct a new range object from a pair of bounds
  ///
  /// Construct a new range object from a pair of values, the first of which
  /// is taken to be the minimum, and the second of which is taken to be the
  /// maximum.
  ///
  /// @param p The pair of values to use as the minimum and maximum
  Range1D(const std::pair<Type, Type>& p) : m_min(p.first), m_max(p.second) {}

  /// @brief Construct a new range object from an existing range
  ///
  /// This simply copies the values from the existing range to the new one.
  /// It's the copy constructor.
  ///
  /// @param o The range to copy
  Range1D(const Range1D<Type>& o) : m_min(o.min()), m_max(o.max()) {}

  /// @brief Set the minimum value
  ///
  /// Override the minimum value of the range, regardless of what was already
  /// set.
  ///
  /// @note If you want to shrink or expand the range, use the shrink and
  /// expand methods.
  ///
  /// @param v The value to use as the new minimum
  void setMin(const Type& v) { m_min = v; }

  /// @brief Set the maximum value
  ///
  /// Override the maximum value of the range, regardless of what was already
  /// set.
  ///
  /// @note If you want to shrink or expand the range, use the shrink and
  /// expand methods.
  ///
  /// @param v The value to use as the new maximum
  void setMax(const Type& v) { m_max = v; }

  /// @brief Set the minimum and maximum value
  ///
  /// Override both the minimum and maximum value of the range, regardless of
  /// what they were set to.
  ///
  /// @note If you want to shrink or expand the range, use the shrink and
  /// expand methods.
  ///
  /// @note After this operation, the range should be exactly equal to [min,
  /// max]
  ///
  /// @param min The new minimum value of the range
  /// @param max The new maximum value of the range
  void set(const Type& min, const Type& max) {
    m_min = min;
    m_max = max;
  }

  /// @brief Shrink a range by increasing the minimum value
  ///
  /// Shrink the range by increasing the minimum value. If the given value is
  /// smaller than the current minimum (in other words, if the proposed new
  /// range would be larger than the current range), this is a no-op.
  ///
  /// @param v The proposed new minimum for the range
  void shrinkMin(const Type& v) { m_min = std::max(m_min, v); }

  /// @brief Shrink a range by decreasing the maximum value
  ///
  /// Shrink the range by decreasing the maximum value. If the given value is
  /// larger than the current maximum (in other words, if the proposed new
  /// range would be larger than the current range), this is a no-op.
  ///
  /// @param v The proposed new maximum for the range
  void shrinkMax(const Type& v) { m_max = std::min(m_max, v); }

  /// @brief Shrink a range on both ends
  ///
  /// Shrink a range by increasing the minimum value as well as decreasing the
  /// maximum value. If either of the values are already smaller or larger
  /// (respectively) than the proposed values, then that particular boundary of
  /// the interval is not shrunk.
  ///
  /// @note After this operation, the range is always equal to or smaller than
  /// [min, max].
  ///
  /// @param min The proposed new minimum for the range
  /// @param max The proposed new maximum for the range
  void shrink(const Type& min, const Type& max) {
    shrinkMin(min);
    shrinkMax(max);
  }

  /// @brief Expand a range by decreasing the minimum value
  ///
  /// Expand the range by decreasing the minimum value. If the given value is
  /// larger than the current minimum (in other words, if the proposed new
  /// range would be smaller than the current range), this is a no-op.
  ///
  /// @param v The proposed new minimum for the range
  void expandMin(const Type& v) { m_min = std::min(m_min, v); }

  /// @brief Expand a range by increasing the maximum value
  ///
  /// Expand the range by increasing the maximum value. If the given value is
  /// smaller than the current maximum (in other words, if the proposed new
  /// range would be smaller than the current range), this is a no-op.
  ///
  /// @param v The proposed new maximum for the range
  void expandMax(const Type& v) { m_max = std::max(m_max, v); }

  /// @brief Expand a range on both ends
  ///
  /// Expand a range by decreasing the minimum value as well as increasing the
  /// maximum value. If either of the values are already larger or smaller
  /// (respectively) than the proposed values, then that particular boundary of
  /// the interval is not expanded.
  ///
  /// @note After this operation, the range is always equal to or larger than
  /// [min, max].
  ///
  /// @param min The proposed new minimum for the range
  /// @param max The proposed new maximum for the range
  void expand(const Type& min, const Type& max) {
    expandMin(min);
    expandMax(max);
  }

  /// @brief Return the minimum value of the range (inclusive)
  Type min(void) const { return m_min; }

  /// @brief Return the maximum value of the range (inclusive)
  Type max(void) const { return m_max; }

  /// @brief Compute the size of the range
  ///
  /// The size of a range is defined as the difference between the minimum and
  /// the maximum. For degenerate ranges, this is zero.
  ///
  /// @warning Due to the nature of numbers, the result of this function can be
  /// somewhat ambiguous. For natural numbers, you could argue that the range
  /// [n, n] has size 0 or size 1. In this case we say it has size 0. The
  /// uncountable nature of the reals means this doesn't matter for them, but
  /// this can be awkward when working with integers.
  ///
  /// @return The size of the range
  Type size(void) const {
    return std::max(static_cast<Type>(0), m_max - m_min);
  }

  /// @brief Determine if this range is degenerate or not
  ///
  /// A degenerate range has a minimum higher than the maximum, and thus
  /// cannot contain any values.
  ///
  /// @return true The range is degenerate and has size zero
  /// @return false The range is not degenerate
  bool degenerate(void) const { return m_min > m_max; }

  /// @brief Determine if the range contains a given value
  ///
  /// A value is inside a range if and only if it is greater than the minimum
  /// and smaller than the maximum.
  ///
  /// @param v The value to check
  ///
  /// @return true The value is inside the range
  /// @return false The value is not inside the range
  bool contains(const Type& v) const { return m_min <= v && v < m_max; }

  /// @brief Determine whether the range intersects another range
  ///
  /// The intersection of a range is the space where both ranges overlap. If
  /// the ranges overlap at all, they are said to intersect. This operation
  /// is commutative.
  ///
  /// @param o The other range to check
  ///
  /// @return true The ranges intersect
  /// @return false The ranges do not intersect
  bool operator&&(const Range1D<Type>& o) const {
    return m_min <= o.max() && o.min() <= m_max;
  }

  /// @brief Determine whether the range is equal to another range
  ///
  /// Two ranges are equal if and only if their minima and maxima are the
  /// same.
  ///
  /// @warning This method relies on the existence of a well-defined notion
  /// of equality for the underlying types. Using this method on floating
  /// ranges may have unintended effecrs.
  ///
  /// @param o The other range to check
  ///
  /// @return true The ranges are equal
  /// @return false The ranges are not equal
  bool operator==(const Range1D<Type>& o) const {
    return min() == o.min() && max() == o.max();
  }

  /// @brief Determine whether the left-hand range is a subset of the
  /// right-hand range
  ///
  /// A range is a subset of another range if and only if all values
  /// contained in the first range are also contained in the second range.
  ///
  /// @param o The other range to check
  ///
  /// @return true The left-hand range is a subset of the right-hand range
  /// @return false The left-hand range is not a subset of the right-hand
  /// range
  bool operator<=(const Range1D<Type>& o) const {
    return min() >= o.min() && max() <= o.max();
  }

  /// @brief Determine whether the left-hand range is a superset of the
  /// right-hand range
  ///
  /// A range is a superset of another range if and only if all values
  /// contained in the second range are also contained in the first range.
  ///
  /// @param o The other range to check
  ///
  /// @return true The left-hand range is a superset of thr right-hand range
  /// @return false The left-hand range is not a superset of the right-hand
  /// range
  bool operator>=(const Range1D<Type>& o) const {
    return min() <= o.min() && max() >= o.max();
  }

  /// @brief Assignment operator
  ///
  /// Copy the right-hand range into the left-hand range, which means setting
  /// the minimum and maximum to equal the minimum and maximum of the
  /// right-hand side.
  ///
  /// @param o The range of values to copy
  ///
  /// @return This range
  Range1D<Type>& operator=(const Range1D<Type>& o) {
    m_min = o.min();
    m_max = o.max();

    return *this;
  }

  /// @brief Compute the intersection of two ranges
  ///
  /// The intersection of two ranges is the range containing all values
  /// contained in both ranges. If the two ranges do not intersect, the
  /// intersection is a degenerate range. This operation is commutative.
  ///
  /// @param o The range to compute the intersection with
  ///
  /// @return The intersection range between the two ranges
  Range1D<Type> operator&(const Range1D<Type>& o) const {
    return Range1D<Type>(std::max(m_min, o.min()), std::min(m_max, o.max()));
  }

  /// @brief Set the range to the intersection of itself and another range
  ///
  /// This is an assignment version of operator&, which updates the range on
  /// which it is called to ensure that the new range is the intersection of
  /// the old range and the new range.
  ///
  /// @param o The range to compute the intersection with
  ///
  /// @return This object
  Range1D<Type>& operator&=(const Range1D<Type>& o) {
    m_min = std::max(m_min, o.min());
    m_max = std::min(m_max, o.max());

    return *this;
  }

 private:
  Type m_min, m_max;
};
}  // namespace Acts
