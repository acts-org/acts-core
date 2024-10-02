// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace Acts {

/// @brief Fast inverse square root function
/// Taken from https://en.wikipedia.org/wiki/Fast_inverse_square_root
/// @param x the number to calculate the inverse square root of
/// @param iterations the number of newton iterations to perform
template <typename T>
constexpr T fastInverseSqrt(T x, int iterations = 1) noexcept {
  static_assert(std::numeric_limits<T>::is_iec559 &&
                "Only IEEE 754 is supported");
  static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                "Only float and double are supported");
  using I = std::conditional_t<std::is_same_v<T, float>, std::uint32_t,
                               std::uint64_t>;

  constexpr I magic =
      std::is_same_v<T, float> ? 0x5f3759df : 0x5fe6eb50c7b537a9;

  union {
    T f;
    I i;
  } u = {x};

  u.i = magic - (u.i >> 1);

  for (int i = 0; i < iterations; ++i) {
    u.f *= 1.5f - 0.5f * x * u.f * u.f;
  }

  return u.f;
}

/// @brief Fast power function @see `std::pow`
/// Taken from
/// https://martin.ankerl.com/2012/01/25/optimized-approximative-pow-in-c-and-cpp
/// @param a the base
/// @param b the exponent
constexpr double fastPow(double a, double b) {
  // enable only on IEEE 754
  static_assert(std::numeric_limits<double>::is_iec559);

  constexpr std::int64_t magic = 0x3FEF127F00000000;

  union {
    double f;
    std::int64_t i;
  } u = {a};

  u.i = static_cast<std::int64_t>(b * (u.i - magic) + magic);

  return u.f;
}

/// @brief Fast power function more precise than @see `fastPow`
/// Taken from
/// https://martin.ankerl.com/2012/01/25/optimized-approximative-pow-in-c-and-cpp
/// @param a the base
/// @param b the exponent
constexpr double fastPowMorePrecise(double a, double b) {
  // binary exponentiation
  double r = 1.0;
  int exp = std::abs(static_cast<int>(b));
  double base = b > 0 ? a : 1 / a;
  while (exp != 0) {
    if ((exp & 1) != 0) {
      r *= base;
    }
    base *= base;
    exp >>= 1;
  }

  return r * fastPow(a, b - static_cast<int>(b));
}

}  // namespace Acts
