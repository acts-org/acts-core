// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <array>
#include <limits>

#include "Eigen/Dense"

namespace Acts::VectorHelpers {

/// Calculate phi (transverse plane angle) from compatible Eigen types
/// @tparam Derived Eigen derived concrete type
/// @param v Any vector like Eigen type, static or dynamic
/// @note Will static assert that the number of rows of @p v is at least 2, or
/// in case of dynamic size, will abort execution if that is not the case.
/// @return The value of the angle in the transverse plane.
template <typename Derived>
double phi(const Eigen::MatrixBase<Derived>& v) noexcept {
  constexpr int rows = Eigen::MatrixBase<Derived>::RowsAtCompileTime;
  if constexpr (rows != -1) {
    // static size, do compile time check
    static_assert(rows >= 2,
                  "Phi function not valid for vectors not at least 2D");
  } else {
    // dynamic size
    assert(v.rows() >= 2 &&
           "Phi function not valid for vectors not at least 2D");
  }
  return std::atan2(v[1], v[0]);
}

/// Calculate phi (transverse plane angle) from anything implementing a method
/// like `phi()` returning anything convertible to `double`.
/// @tparam T anything that has a phi method
/// @param v Any type that implements a phi method
/// @return The phi value
template <typename T>
double phi(const T& v) noexcept
  requires requires {
    { v.phi() } -> std::floating_point;
  }
{
  return v.phi();
}

/// Calculate radius in the transverse (xy) plane of a vector
/// @tparam Derived Eigen derived concrete type
/// @param v Any vector like Eigen type, static or dynamic
/// @note Will static assert that the number of rows of @p v is at least 2, or
/// in case of dynamic size, will abort execution if that is not the case.
/// @return The transverse radius value.
template <typename Derived>
double perp(const Eigen::MatrixBase<Derived>& v) noexcept {
  constexpr int rows = Eigen::MatrixBase<Derived>::RowsAtCompileTime;
  if constexpr (rows != -1) {
    // static size, do compile time check
    static_assert(rows >= 2,
                  "Perp function not valid for vectors not at least 2D");
  } else {
    // dynamic size
    assert(v.rows() >= 2 &&
           "Perp function not valid for vectors not at least 2D");
  }
  return v.template head<2>().norm();
}

/// Calculate the theta angle (longitudinal w.r.t. z axis) of a vector
/// @tparam Derived Eigen derived concrete type
/// @param v Any vector like Eigen type, static or dynamic
/// @note Will static assert that the number of rows of @p v is at least 3, or
/// in case of dynamic size, will abort execution if that is not the case.
/// @return The theta value
template <typename Derived>
double theta(const Eigen::MatrixBase<Derived>& v) noexcept {
  constexpr int rows = Eigen::MatrixBase<Derived>::RowsAtCompileTime;
  if constexpr (rows != -1) {
    // static size, do compile time check
    static_assert(rows == 3, "Theta function not valid for non-3D vectors.");
  } else {
    // dynamic size
    assert(v.rows() == 3 && "Theta function not valid for non-3D vectors.");
  }

  return std::atan2(perp(v), v[2]);
}

/// Calculate the pseudorapidity for a vector.
/// @tparam Derived Eigen derived concrete type
/// @param v Any vector like Eigen type, static or dynamic
/// @note Will static assert that the number of rows of @p v is at least 3, or
/// in case of dynamic size, will abort execution if that is not the case.
/// @return The pseudorapidity value
template <typename Derived>
double eta(const Eigen::MatrixBase<Derived>& v) noexcept {
  constexpr int rows = Eigen::MatrixBase<Derived>::RowsAtCompileTime;
  if constexpr (rows != -1) {
    // static size, do compile time check
    static_assert(rows == 3, "Eta function not valid for non-3D vectors.");
  } else {
    // dynamic size
    assert(v.rows() == 3 && "Eta function not valid for non-3D vectors.");
  }

  if (v[0] == 0. && v[1] == 0.) {
    return std::copysign(std::numeric_limits<double>::infinity(), v[2]);
  } else {
    return std::asinh(v[2] / perp(v));
  }
}

/// @brief Fast evaluation of trigonomic functions.
///
/// @param direction for this evaluatoin
///
/// @return cos(phi), sin(phi), cos(theta), sin(theta), 1/sin(theta)
inline std::array<ActsScalar, 4> evaluateTrigonomics(const Vector3& direction) {
  const ActsScalar x = direction(0);  // == cos(phi) * sin(theta)
  const ActsScalar y = direction(1);  // == sin(phi) * sin(theta)
  const ActsScalar z = direction(2);  // == cos(theta)
  // can be turned into cosine/sine
  const ActsScalar cosTheta = z;
  const ActsScalar sinTheta = std::sqrt(1 - z * z);
  assert(sinTheta != 0 &&
         "VectorHelpers: Vector is parallel to the z-axis "
         "which leads to division by zero");
  const ActsScalar invSinTheta = 1. / sinTheta;
  const ActsScalar cosPhi = x * invSinTheta;
  const ActsScalar sinPhi = y * invSinTheta;

  return {cosPhi, sinPhi, cosTheta, sinTheta};
}

/// Helper method to extract the binning value from a 3D vector.
///
/// For this method a 3D vector is required to guarantee all potential
/// binning values.
inline double cast(const Vector3& position, BinningValue bval) {
  switch (bval) {
    case BinningValue::binX:
      return position[0];
    case BinningValue::binY:
      return position[1];
    case BinningValue::binZ:
      return position[2];
    case BinningValue::binR:
      return perp(position);
    case BinningValue::binPhi:
      return phi(position);
    case BinningValue::binRPhi:
      return perp(position) * phi(position);
    case BinningValue::binH:
      return theta(position);
    case BinningValue::binEta:
      return eta(position);
    case BinningValue::binMag:
      return position.norm();
    default:
      assert(false && "Invalid BinningValue enum value");
      return std::numeric_limits<double>::quiet_NaN();
  }
}

/// @brief Calculates column-wise cross products of a matrix and a vector and
/// stores the result column-wise in a matrix.
///
/// @param [in] m Matrix that will be used for cross products
/// @param [in] v Vector for cross products
/// @return Constructed matrix
inline ActsMatrix<3, 3> cross(const ActsMatrix<3, 3>& m, const Vector3& v) {
  ActsMatrix<3, 3> r;
  r.col(0) = m.col(0).cross(v);
  r.col(1) = m.col(1).cross(v);
  r.col(2) = m.col(2).cross(v);

  return r;
}

/// Access the three-position components in a four-position vector.
inline auto position(const Vector4& pos4) {
  return pos4.segment<3>(ePos0);
}

/// Access the three-position components in a free parameters vector.
inline auto position(const FreeVector& params) {
  return params.segment<3>(eFreePos0);
}

/// Construct a four-vector from a three-vector and scalar fourth component.
template <typename vector3_t>
inline auto makeVector4(const Eigen::MatrixBase<vector3_t>& vec3,
                        typename vector3_t::Scalar w)
    -> Eigen::Matrix<typename vector3_t::Scalar, 4, 1> {
  EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(vector3_t, 3);

  Eigen::Matrix<typename vector3_t::Scalar, 4, 1> vec4;
  vec4[ePos0] = vec3[ePos0];
  vec4[ePos1] = vec3[ePos1];
  vec4[ePos2] = vec3[ePos2];
  vec4[eTime] = w;
  return vec4;
}

/// Calculate the incident angles of a vector with in a given reference frame
/// @tparam Derived Eigen derived concrete type
/// @param direction The crossing direction in the global frame
/// @param globalToLocal Rotation from global to local frame
/// @return The angles of incidence in the two normal planes
inline std::pair<double, double> incidentAngles(
    const Acts::Vector3& direction,
    const Acts::RotationMatrix3& globalToLocal) {
  Acts::Vector3 trfDir = globalToLocal * direction;
  // The angles are defined with respect to the measurement axis
  // i.e. "head-on" == pi/2, parallel = 0
  double phi = std::atan2(trfDir[2], trfDir[0]);
  double theta = std::atan2(trfDir[2], trfDir[1]);
  return {phi, theta};
}

}  // namespace Acts::VectorHelpers
