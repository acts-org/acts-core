// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/EnumBitwiseOperators.hpp"

#include <cstdint>
#include <limits>
#include <ostream>
#include <type_traits>

namespace Acts {

/// Collection of bit masks to enable steering which components of a track state
/// should be initialized, and which should be left invalid.
/// These mask values can be combined using binary operators, so
/// (TrackStatePropMask::Predicted | TrackStatePropMask::Jacobian) will instruct
/// allocating storage for both predicted parameters (including covariance) and
/// a jacobian.
/// The enum is used as a strong type wrapper around the bits to prevent
/// autoconversion from integer
enum struct TrackStatePropMask : std::uint8_t {
  None = 0,
  Predicted = 1 << 0,
  Filtered = 1 << 1,
  Smoothed = 1 << 2,
  Jacobian = 1 << 3,
  Calibrated = 1 << 4,

  All = std::numeric_limits<std::uint8_t>::max(),  // should be all ones
};

ACTS_DEFINE_ENUM_BITWISE_OPERATORS(TrackStatePropMask)

std::ostream& operator<<(std::ostream& os, TrackStatePropMask mask);

}  // namespace Acts
