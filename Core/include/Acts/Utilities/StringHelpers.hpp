// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <iomanip>
#include <iostream>
#include <string>

#include "Eigen/Dense"

namespace Acts {

namespace detail {
inline double roundWithPrecision(double val, int precision) {
  if (val < 0 && std::abs(val) * std::pow(10, precision) < 1.) {
    return -val;
  }
  return val;
}
}  // namespace detail

/// Print out a matrix in a structured way.
///
/// @tparam derived_t Type of the matrix
/// @param matrix The matrix to print
/// @param precision Numeric output precision
/// @param offset Offset in front of matrix lines
/// @return The printed string
template <typename derived_t>
inline std::string toString(const Eigen::MatrixBase<derived_t>& matrix,
                            int precision = 4, const std::string& offset = "") {
  std::ostringstream sout;

  sout << std::setiosflags(std::ios::fixed) << std::setprecision(precision);
  if (matrix.cols() == 1) {
    sout << "(";
    for (int i = 0; i < matrix.rows(); ++i) {
      double val = detail::roundWithPrecision(matrix(i, 0), precision);
      sout << val;
      if (i != matrix.rows() - 1) {
        sout << ", ";
      }
    }
    sout << ")";
  } else {
    for (int i = 0; i < matrix.rows(); ++i) {
      for (int j = 0; j < matrix.cols(); ++j) {
        if (j == 0) {
          sout << "(";
        }
        double val = detail::roundWithPrecision(matrix(i, j), precision);
        sout << val;
        if (j == matrix.cols() - 1) {
          sout << ")";
        } else {
          sout << ", ";
        }
      }
      if (i != matrix.rows() -
                   1) {  // make the end line and the offset in the next line
        sout << std::endl;
        sout << offset;
      }
    }
  }
  return sout.str();
}

/// Print out a translation in a structured way.
/// @param translation The translation to print
/// @param precision Numeric output precision
/// @return The printed string
inline std::string toString(const Acts::Translation3& translation,
                            int precision = 4) {
  Acts::Vector3 trans;
  trans[0] = translation.x();
  trans[1] = translation.y();
  trans[2] = translation.z();
  return toString(trans, precision);
}

/// Print out a transform in a structured way.
/// @param transform The transform to print
/// @param precision Numeric output precision
/// @param offset Offset in front of matrix lines
/// @return The printed string
inline std::string toString(const Acts::Transform3& transform,
                            int precision = 4, const std::string& offset = "") {
  std::ostringstream sout;
  sout << "Translation : " << toString(transform.translation(), precision)
       << std::endl;
  std::string rotationOffset = offset + "              ";
  sout << offset << "Rotation    : "
       << toString(transform.rotation(), precision + 2, rotationOffset);
  return sout.str();
}

/// Print out a vector of ActsScalar
/// @param pVector The vector to print
/// @param precision Numeric output precision
inline std::string toString(const std::vector<ActsScalar>& pVector,
                            int precision = 4) {
  std::ostringstream sout;
  sout << std::setiosflags(std::ios::fixed) << std::setprecision(precision);
  sout << "(";
  for (const auto [i, val] : enumerate(pVector)) {
    sout << val;
    if (i != pVector.size() - 1) {
      sout << ", ";
    }
  }
  sout << ")";
  return sout.str();
}

}  // namespace Acts
