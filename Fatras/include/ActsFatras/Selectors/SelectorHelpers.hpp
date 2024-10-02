// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Selectors/detail/combine_selectors.hpp"

#include <functional>
#include <limits>

namespace ActsFatras {

/// Select all objects with an extracted value equal or larger than the cut.
template <typename cast_t>
struct Min {
  double valMin = 0.;

  template <typename T>
  bool operator()(const T &thing) const {
    return (valMin <= cast_t()(thing));
  }
};

/// Select all objects with an extracted value below the cut.
template <typename cast_t>
struct Max {
  double valMax = std::numeric_limits<double>::max();

  template <typename T>
  bool operator()(const T &thing) const {
    return (cast_t()(thing) < valMax);
  }
};

/// Select all objects with an extracted value within the range.
///
/// The range is defined as the left, half-open interval within the cuts.
template <typename cast_t>
struct Range {
  double valMin = std::numeric_limits<double>::lowest();
  double valMax = std::numeric_limits<double>::max();

  template <typename T>
  bool operator()(const T &thing) const {
    const auto val = cast_t()(thing);
    return ((valMin <= val) && (val < valMax));
  }
};

/// Select objects that fulfill all selectors.
template <typename... selectors_t>
using CombineAnd =
    detail::CombineSelectors<true, std::logical_and<bool>, selectors_t...>;

/// Select objects that fulfill at least one selector.
template <typename... selectors_t>
using CombineOr =
    detail::CombineSelectors<false, std::logical_or<bool>, selectors_t...>;

}  // namespace ActsFatras
