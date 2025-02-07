// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cmath>
#include <numbers>

namespace Acts::detail {

/// Wrap a periodic value back into the nominal range.
template <typename T>
inline T wrap_periodic(T value, T start, T range) {
  using std::floor;
  // only wrap if really necessary
  T diff = value - start;
  return ((0 <= diff) && (diff < range))
             ? value
             : (value - range * floor(diff / range));
}

/// Compute the minimal `lhs - rhs` using the periodicity.
///
/// Imagine you have two values within the nominal range: `l` is close to the
/// lower edge and `u` is close to the upper edge. The naive difference between
/// the two is almost as large as the range itself. If we move `l` to its
/// equivalent value outside the nominal range, i.e. just above the upper edge,
/// the effective absolute difference becomes smaller.
///
/// @note The sign of the returned value can be different from `lhs - rhs`
template <typename T>
inline T difference_periodic(T lhs, T rhs, T range) {
  using std::fmod;
  T delta = fmod(lhs - rhs, range);
  // check if |delta| is larger than half the range. if that is the case, we
  // can move either rhs/lhs by one range/period to get a smaller |delta|.
  if ((2 * delta) < -range) {
    delta += range;
  } else if (range <= (2 * delta)) {
    delta -= range;
  }
  return delta;
}

/// Calculate the equivalent angle in the [0, 2*pi) range.
template <typename T>
inline T radian_pos(T x) {
  return wrap_periodic<T>(x, T{0}, T{2 * std::numbers::pi});
}

/// Calculate the equivalent angle in the [-pi, pi) range.
template <typename T>
inline T radian_sym(T x) {
  return wrap_periodic<T>(x, -std::numbers::pi_v<T>, T{2 * std::numbers::pi});
}

/// Ensure both phi and theta direction angles are within the allowed range.
///
/// @param[in] phi Transverse direction angle
/// @param[in] theta Longitudinal direction angle
/// @return pair<phi,theta> containing the updated angles
///
/// The phi angle is truly cyclic, i.e. all values outside the nominal range
/// [-pi,pi) have a corresponding value inside nominal range, independent from
/// the theta angle. The theta angle is more complicated. Imagine that the two
/// angles describe a position on the unit sphere. If theta moves outside its
/// nominal range [0,pi], we are moving over one of the two poles of the unit
/// sphere along the great circle defined by phi. The angles still describe a
/// valid position on the unit sphere, but to describe it with angles within
/// their nominal range, both phi and theta need to be updated; when moving over
/// the poles, phi needs to be flipped by 180degree to allow theta to remain
/// within its nominal range.
template <typename T>
inline std::pair<T, T> normalizePhiTheta(T phi, T theta) {
  // wrap to [0,2pi). while the nominal range of theta is [0,pi], it is
  // periodic, i.e. describes identical positions, in the full [0,2pi) range.
  // moving it first to the periodic range simplifies further steps as the
  // possible range of theta becomes fixed.
  theta = radian_pos(theta);
  if (std::numbers::pi_v<T> < theta) {
    // theta is in the second half of the great circle and outside its nominal
    // range. need to change both phi and theta to be within range.
    phi += std::numbers::pi_v<T>;
    theta = T{2 * std::numbers::pi} - theta;
  }
  return {radian_sym(phi), theta};
}

}  // namespace Acts::detail
