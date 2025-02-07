// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <Eigen/Dense>

namespace Acts::Concepts {
/// @brief Concept that is true iff T is a valid Eigen dense base.
template <typename T>
concept is_eigen_base = requires {
  { T::RowsAtCompileTime };
  { T::ColsAtCompileTime };
};

/// @brief Concept that is true iff T is a valid Eigen dense base with fixed
/// size.
template <typename T>
concept eigen_base_is_fixed_size =
    is_eigen_base<T> && Eigen::PlainObjectBase<T>::RowsAtCompileTime > 0 &&
    Eigen::PlainObjectBase<T>::ColsAtCompileTime > 0;

/// @brief Concept that is true iff T is a valid Eigen dense base with fixed,
/// square size.
template <typename T>
concept eigen_base_is_square = eigen_base_is_fixed_size<T> &&
                               Eigen::PlainObjectBase<T>::RowsAtCompileTime ==
                                   Eigen::PlainObjectBase<T>::ColsAtCompileTime;

/// @brief Concept that is true iff T1 and T2 have the same, known at compile
/// time, number of rows.
template <typename T1, typename T2>
concept eigen_bases_have_same_num_rows =
    eigen_base_is_fixed_size<T1> && eigen_base_is_fixed_size<T2> &&
    static_cast<std::size_t>(Eigen::PlainObjectBase<T1>::RowsAtCompileTime) ==
        static_cast<std::size_t>(Eigen::PlainObjectBase<T2>::RowsAtCompileTime);

/// @brief Concept that is true iff T1 and T2 have the same, known at compile
/// time, number of columns.
template <typename T1, typename T2>
concept eigen_bases_have_same_num_cols =
    eigen_base_is_fixed_size<T1> && eigen_base_is_fixed_size<T2> &&
    static_cast<std::size_t>(Eigen::PlainObjectBase<T1>::ColsAtCompileTime) ==
        static_cast<std::size_t>(Eigen::PlainObjectBase<T2>::ColsAtCompileTime);

/// @brief Concept that is true iff T1 and T2 have the same, known at compile
/// time, size.
template <typename T1, typename T2>
concept eigen_bases_have_same_size = eigen_bases_have_same_num_rows<T1, T2> &&
                                     eigen_bases_have_same_num_cols<T1, T2>;
}  // namespace Acts::Concepts
