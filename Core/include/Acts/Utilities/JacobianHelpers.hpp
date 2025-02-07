// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

namespace Acts {

/// @brief Calculates the Jacobian for spherical to free
///        direction vector transformation
///
/// @note We use the direction vector as an input because
///       the trigonometric simplify that way
///
/// @param direction The normalised direction vector
///
/// @return The Jacobian d(dir_x, dir_y, dir_z) / d(phi, theta)
///
inline ActsMatrix<3, 2> sphericalToFreeDirectionJacobian(
    const Vector3& direction) {
  auto [cosPhi, sinPhi, cosTheta, sinTheta] =
      VectorHelpers::evaluateTrigonomics(direction);

  // clang-format off
  ActsMatrix<3, 2> jacobian;
  jacobian <<
    -direction.y(),  cosTheta * cosPhi,
     direction.x(),  cosTheta * sinPhi,
     0,             -sinTheta;
  // clang-format on

  return jacobian;
}

/// @brief Calculates the Jacobian for free to spherical
///        direction vector transformation
///
/// @note We use the direction vector as an input because
///       the trigonometric simplify that way
///
/// @param direction The normalised direction vector
///
/// @return The Jacobian d(phi, theta) / d(dir_x, dir_y, dir_z)
///
inline ActsMatrix<2, 3> freeToSphericalDirectionJacobian(
    const Vector3& direction) {
  auto [cosPhi, sinPhi, cosTheta, sinTheta] =
      VectorHelpers::evaluateTrigonomics(direction);
  double invSinTheta = 1. / sinTheta;

  // clang-format off
  ActsMatrix<2, 3> jacobian;
  jacobian <<
    -sinPhi * invSinTheta, cosPhi * invSinTheta, 0,
     cosPhi * cosTheta,    sinPhi * cosTheta,    -sinTheta;
  // clang-format on

  return jacobian;
}

}  // namespace Acts
