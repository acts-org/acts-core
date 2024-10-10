// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
