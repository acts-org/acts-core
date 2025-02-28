// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// for GNU: ignore this specific warning, otherwise just include Eigen/Dense
#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#if __GNUC__ >= 12
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include <Eigen/Core>
#include <Eigen/Geometry>
#pragma GCC diagnostic pop
#else
#include <Eigen/Core>
#include <Eigen/Geometry>
#endif

#include <concepts>
#include <type_traits>

namespace Acts {

/// @defgroup acts-algebra-types Vector/matrix types with a common scalar type
///
/// These are the default vector/matrix types that should be used throughout the
/// codebase. They all use the common Acts scalar type but support variable size
/// either at compile- or runtime.
///
/// Eigen does not have a distinct type for symmetric matrices. A typedef for
/// fixed-size matrices is still defined to simplify definition (one template
/// size vs two template size for generic matrices) and to clarify semantic
/// meaning in interfaces. It also ensures that the matrix is square. However,
/// the user is responsible for ensuring that the values are symmetric.
///
/// Without a distinct type for symmetric matrices, there is no way to provide
/// any conditions e.g. square size, for the dynamic-sized case. Consequently,
/// no dynamic-sized symmetric matrix type is defined. Use the
/// `ActsDynamicMatrix` instead.

template <unsigned int kSize>
using ActsVector = Eigen::Matrix<double, kSize, 1>;

template <unsigned int kRows, unsigned int kCols>
using ActsMatrix = Eigen::Matrix<double, kRows, kCols>;

template <unsigned int kSize>
using ActsSquareMatrix = Eigen::Matrix<double, kSize, kSize>;

using ActsDynamicVector = Eigen::Matrix<double, Eigen::Dynamic, 1>;

using ActsDynamicMatrix = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;

/// @defgroup coordinates-types Fixed-size vector/matrix types for coordinates
///
/// These predefined types should always be used when handling coordinate
/// vectors in different coordinate systems, i.e. on surfaces (2d), spatial
/// position (3d), or space-time (4d).
///

// coordinate vectors
using Vector2 = ActsVector<2>;
using Vector3 = ActsVector<3>;
using Vector4 = ActsVector<4>;

// square matrices e.g. for coordinate covariance matrices
using SquareMatrix2 = ActsSquareMatrix<2>;
using SquareMatrix3 = ActsSquareMatrix<3>;
using SquareMatrix4 = ActsSquareMatrix<4>;

// pure translation transformations
using Translation2 = Eigen::Translation<double, 2>;
using Translation3 = Eigen::Translation<double, 3>;

// linear (rotation) matrices
using RotationMatrix2 = SquareMatrix2;
using RotationMatrix3 = SquareMatrix3;

// pure rotation defined by a rotation angle around a rotation axis
using AngleAxis3 = Eigen::AngleAxis<double>;

// combined affine transformations. types are chosen for better data alignment:
// - 2d affine compact stored as 2x3 matrix
// - 3d affine stored as 4x4 matrix
using Transform2 = Eigen::Transform<double, 2, Eigen::AffineCompact>;
using Transform3 = Eigen::Transform<double, 3, Eigen::Isometry>;

constexpr double s_transformEquivalentTolerance = 1e-9;
constexpr double s_isometryEquivalentTolerance = 1e-6;

// Check that the linear part of a transform is a pure rotation
template <typename T>
inline bool isIsometry(const T& transform) {
  bool isDetOne = false;
  bool isOrthogonal = false;
  using BaseT = std::remove_cvref_t<T>;

  if constexpr (std::same_as<BaseT, Transform3>) {
    auto R = transform.linear();
    isDetOne = std::abs(R.determinant() - 1.0) < s_isometryEquivalentTolerance;
    isOrthogonal =
        (R.transpose() * R).isIdentity(s_isometryEquivalentTolerance);

  } else if constexpr (std::same_as<BaseT, RotationMatrix3>) {
    isDetOne =
        std::abs(transform.determinant() - 1.0) < s_isometryEquivalentTolerance;
    isOrthogonal = (transform.transpose() * transform)
                       .isIdentity(s_isometryEquivalentTolerance);

  } else {
    return false;
  }

  return (isDetOne && isOrthogonal);
}

}  // namespace Acts
