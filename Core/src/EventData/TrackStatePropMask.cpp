// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/EventData/TrackStatePropMask.hpp"

#include "Acts/Utilities/Helpers.hpp"

#include <ostream>

namespace Acts {

std::ostream& operator<<(std::ostream& os, TrackStatePropMask mask) {
  using PM = TrackStatePropMask;
  os << "TrackStatePropMask(";
  if (mask == PM::None) {
    os << "None";
  } else {
    os << "\n  [" << (ACTS_CHECK_BIT(mask, PM::Predicted) ? "x" : " ")
       << "] predicted";
    os << "\n  [" << (ACTS_CHECK_BIT(mask, PM::Filtered) ? "x" : " ")
       << "] filtered";
    os << "\n  [" << (ACTS_CHECK_BIT(mask, PM::Smoothed) ? "x" : " ")
       << "] smoothed";
    os << "\n  [" << (ACTS_CHECK_BIT(mask, PM::Jacobian) ? "x" : " ")
       << "] jacobian";
    os << "\n  [" << (ACTS_CHECK_BIT(mask, PM::Calibrated) ? "x" : " ")
       << "] calibrated";
    os << "\n";
  }
  os << ")";
  return os;
}

}  // namespace Acts
