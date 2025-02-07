// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cmath>

namespace Acts {

template <typename T>
constexpr auto square(T x) {
  return x * x;
}

template <typename... T>
constexpr auto hypotSquare(T... args) {
  return (square(args) + ...);
}

template <typename... T>
constexpr auto fastHypot(T... args) {
  return std::sqrt(hypotSquare(args...));
}

}  // namespace Acts
