// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <string>
#include <vector>

namespace Acts {

struct TGeoPrimitivesHelper {
  /// Helper method to create a transform from
  /// Rotation matrix vectors:
  /// @param rotationMatrixCol0
  /// @param rotationMatrixCol1
  /// @param rotationMatrixCol2
  /// And translation
  /// @param translation
  static inline Transform3 makeTransform(
      const Eigen::Vector3d& rotationMatrixCol0,
      const Eigen::Vector3d& rotationMatrixCol1,
      const Eigen::Vector3d& rotationMatrixCol2,
      const Eigen::Vector3d& translation) {
    Transform3 trf;
    trf.matrix().block(0, 0, 3, 1) = rotationMatrixCol0;
    trf.matrix().block(0, 1, 3, 1) = rotationMatrixCol1;
    trf.matrix().block(0, 2, 3, 1) = rotationMatrixCol2;
    trf.matrix().block(0, 3, 3, 1) = translation;
    return trf;
  }

  /// Private helper method : match string with wildcards
  /// @param first is the one with the potential wildcard
  /// @param second is the test string
  static bool match(const char* first, const char* second);

  /// Private helper method : match string with wildcards
  /// Method that uses the match method with wild cards and
  /// performs it on an input list
  /// @param first is the one with the potential wildcard
  /// @param second is the test string
  static bool match(const std::vector<std::string>& first, const char* second);
};
}  // namespace Acts
