// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/detail/AlignmentHelper.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace Acts::Test {

/// Test for rotation matrix and calculation of derivative of rotated x/y/z axis
/// w.r.t. rotation parameters
BOOST_AUTO_TEST_CASE(alignment_helper_test) {
  // (a) Test with non-identity rotation matrix
  // Rotation angle parameters
  const double alpha = std::numbers::pi;
  const double beta = 0.;
  const double gamma = std::numbers::pi / 2.;
  // rotation around x axis
  AngleAxis3 rotX(alpha, Vector3(1., 0., 0.));
  // rotation around y axis
  AngleAxis3 rotY(beta, Vector3(0., 1., 0.));
  // rotation around z axis
  AngleAxis3 rotZ(gamma, Vector3(0., 0., 1.));
  double sz = std::sin(gamma);
  double cz = std::cos(gamma);
  double sy = std::sin(beta);
  double cy = std::cos(beta);
  double sx = std::sin(alpha);
  double cx = std::cos(alpha);

  // Calculate the expected rotation matrix for rotZ * rotY * rotX,
  // (i.e. first rotation around x axis, then y axis, last z axis):
  // [ cz*cy  cz*sy*sx-cx*sz  sz*sx+cz*cx*sy ]
  // [ cy*sz  cz*cx+sz*sy*sx  cx*sz*sy-cz*sx ]
  // [ -sy    cy*sx           cy*cx          ]
  RotationMatrix3 expRot = RotationMatrix3::Zero();
  expRot.col(0) = Vector3(cz * cy, cy * sz, -sy);
  expRot.col(1) =
      Vector3(cz * sy * sx - cx * sz, cz * cx + sz * sy * sx, cy * sx);
  expRot.col(2) =
      Vector3(sz * sx + cz * cx * sy, cx * sz * sy - cz * sx, cy * cx);

  // Calculate the expected derivative of local x axis to its rotation
  RotationMatrix3 expRotToXAxis = RotationMatrix3::Zero();
  expRotToXAxis.col(0) = expRot * Vector3(0, 0, 0);
  expRotToXAxis.col(1) = expRot * Vector3(0, 0, -1);
  expRotToXAxis.col(2) = expRot * Vector3(0, 1, 0);

  // Calculate the expected derivative of local y axis to its rotation
  RotationMatrix3 expRotToYAxis = RotationMatrix3::Zero();
  expRotToYAxis.col(0) = expRot * Vector3(0, 0, 1);
  expRotToYAxis.col(1) = expRot * Vector3(0, 0, 0);
  expRotToYAxis.col(2) = expRot * Vector3(-1, 0, 0);

  // Calculate the expected derivative of local z axis to its rotation
  RotationMatrix3 expRotToZAxis = RotationMatrix3::Zero();
  expRotToZAxis.col(0) = expRot * Vector3(0, -1, 0);
  expRotToZAxis.col(1) = expRot * Vector3(1, 0, 0);
  expRotToZAxis.col(2) = expRot * Vector3(0, 0, 0);

  // Construct a transform
  Translation3 translation(Vector3(0., 0., 0.));
  Transform3 transform(translation);
  // Rotation with rotZ * rotY * rotX
  transform *= rotZ;
  transform *= rotY;
  transform *= rotX;
  // Get the rotation of the transform
  const auto rotation = transform.rotation();

  // Check if the rotation matrix is as expected
  CHECK_CLOSE_ABS(rotation, expRot, 1e-15);

  // Call the alignment helper to calculate the derivative of local frame axes
  // w.r.t its rotation
  const auto& [rotToLocalXAxis, rotToLocalYAxis, rotToLocalZAxis] =
      detail::rotationToLocalAxesDerivative(rotation);

  // Check if the derivative for local x axis is as expected
  CHECK_CLOSE_ABS(rotToLocalXAxis, expRotToXAxis, 1e-15);

  // Check if the derivative for local y axis is as expected
  CHECK_CLOSE_ABS(rotToLocalYAxis, expRotToYAxis, 1e-15);

  // Check if the derivative for local z axis is as expected
  CHECK_CLOSE_ABS(rotToLocalZAxis, expRotToZAxis, 1e-15);

  // (b) Test with identity rotation matrix
  RotationMatrix3 iRotation = RotationMatrix3::Identity();

  // Call the alignment helper to calculate the derivative of local frame axes
  // w.r.t its rotation
  const auto& [irotToLocalXAxis, irotToLocalYAxis, irotToLocalZAxis] =
      detail::rotationToLocalAxesDerivative(iRotation);

  // The expected derivatives
  expRotToXAxis << 0, 0, 0, 0, 0, 1, 0, -1, 0;
  expRotToYAxis << 0, 0, -1, 0, 0, 0, 1, 0, 0;
  expRotToZAxis << 0, 1, 0, -1, 0, 0, 0, 0, 0;

  // Check if the derivative for local x axis is as expected
  CHECK_CLOSE_ABS(irotToLocalXAxis, expRotToXAxis, 1e-15);

  // Check if the derivative for local y axis is as expected
  CHECK_CLOSE_ABS(irotToLocalYAxis, expRotToYAxis, 1e-15);

  // Check if the derivative for local z axis is as expected
  CHECK_CLOSE_ABS(irotToLocalZAxis, expRotToZAxis, 1e-15);
}
}  // namespace Acts::Test
