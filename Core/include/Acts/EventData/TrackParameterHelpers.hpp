// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <limits>

namespace Acts {

/// Check if a bound vector is valid. This checks the following:
/// - All values are finite
/// - (optionally) The phi value is in the range [-pi, pi)
/// - (optionally) The theta value is in the range [0, pi]
///
/// @param v The bound vector to check
/// @param validateAngleRange If true, the phi and theta values are range checked
/// @param epsilon The epsilon to use for the checks
/// @param maxAbsEta The maximum allowed eta value
///
/// @return True if the bound vector is valid
bool isBoundVectorValid(
    const BoundVector& v, bool validateAngleRange, double epsilon = 1e-6,
    double maxAbsEta = std::numeric_limits<double>::infinity());

/// Check if a free vector is valid. This checks the following:
/// - All values are finite
/// - Direction is normalized
///
/// @param v The free vector to check
/// @param epsilon The epsilon to use for the checks
/// @param maxAbsEta The maximum allowed eta value
///
/// @return True if the free vector is valid
bool isFreeVectorValid(
    const FreeVector& v, double epsilon = 1e-6,
    double maxAbsEta = std::numeric_limits<double>::infinity());

/// Normalize the bound parameter angles
///
/// @param boundParams The bound parameters to normalize
///
/// @return The normalized bound parameters
inline BoundVector normalizeBoundParameters(const BoundVector& boundParams) {
  BoundVector result = boundParams;
  std::tie(result[eBoundPhi], result[eBoundTheta]) =
      detail::normalizePhiTheta(result[eBoundPhi], result[eBoundTheta]);
  return result;
}

/// Add bound parameters and take care of angle periodicity for phi and theta.
/// This is intended for small differences only i.e. KF updates.
///
/// @param lhs The left hand side bound parameters
/// @param rhs The right hand side bound parameters
///
/// @return The sum of the bound parameters
inline BoundVector addBoundParameters(const BoundVector& lhs,
                                      const BoundVector& rhs) {
  return normalizeBoundParameters(lhs + rhs);
}

/// Subtract bound parameters and take care of angle periodicity for phi and
/// theta. This is intended for small differences only i.e. KF updates.
///
/// @param lhs The left hand side bound parameters
/// @param rhs The right hand side bound parameters
///
/// @return The difference of the bound parameters
inline BoundVector subtractBoundParameters(const BoundVector& lhs,
                                           const BoundVector& rhs) {
  BoundVector result = lhs - rhs;
  result[eBoundPhi] = detail::radian_sym(result[eBoundPhi]);
  result[eBoundTheta] = detail::radian_sym(result[eBoundTheta]);
  return result;
}

}  // namespace Acts
