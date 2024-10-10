// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/detail/MergeHelper.hpp"

namespace Acts::detail {

std::tuple<ActsScalar, ActsScalar, bool> mergedPhiSector(
    ActsScalar hlPhi1, ActsScalar avgPhi1, ActsScalar hlPhi2,
    ActsScalar avgPhi2, const Logger& logger, ActsScalar tolerance) {
  using namespace Acts::UnitLiterals;

  if (std::abs(hlPhi1 - M_PI / 2.0) < tolerance &&
      std::abs(hlPhi2 - M_PI / 2.0) < tolerance) {
    ACTS_VERBOSE("Both phi sectors cover a half circle");

    ACTS_VERBOSE("-> distance between sectors: "
                 << detail::difference_periodic(avgPhi1, avgPhi2, 2 * M_PI) /
                        1_degree);

    if (std::abs(
            std::abs(detail::difference_periodic(avgPhi1, avgPhi2, 2 * M_PI)) -
            M_PI) > tolerance) {
      throw std::invalid_argument(
          "Phi sectors cover half a circle but are not opposite");
    }

    ActsScalar newAvgPhi = detail::radian_sym(avgPhi1 + M_PI / 2.0);
    ActsScalar newHlPhi = M_PI;
    ACTS_VERBOSE("merged: ["
                 << detail::radian_sym(newAvgPhi - newHlPhi) / 1_degree << ", "
                 << detail::radian_sym(newAvgPhi + newHlPhi) / 1_degree
                 << "] ~> " << newAvgPhi / 1_degree << " +- "
                 << newHlPhi / 1_degree);
    return {newHlPhi, newAvgPhi, false};
  }

  ActsScalar minPhi1 = detail::radian_sym(-hlPhi1 + avgPhi1);
  ActsScalar maxPhi1 = detail::radian_sym(hlPhi1 + avgPhi1);

  ACTS_VERBOSE("one: [" << minPhi1 / 1_degree << ", " << maxPhi1 / 1_degree
                        << "] ~> " << avgPhi1 / 1_degree << " +- "
                        << hlPhi1 / 1_degree);

  ActsScalar maxPhi2 = detail::radian_sym(hlPhi2 + avgPhi2);
  ActsScalar minPhi2 = detail::radian_sym(-hlPhi2 + avgPhi2);

  ACTS_VERBOSE("two: [" << minPhi2 / 1_degree << ", " << maxPhi2 / 1_degree
                        << "] ~> " << avgPhi2 / 1_degree << " +- "
                        << hlPhi2 / 1_degree);

  ACTS_VERBOSE("Checking for CCW or CW ordering");
  auto same = [tolerance](ActsScalar a, ActsScalar b) {
    return std::abs(a - b) < tolerance;
  };

  ActsScalar newMaxPhi{}, newMinPhi{};
  ActsScalar newHlPhi = hlPhi1 + hlPhi2;

  bool reversed = false;
  if (same(minPhi1, maxPhi2)) {
    ACTS_VERBOSE("-> CCW ordering: one is 'right' of two");

    newMinPhi = minPhi2;
    newMaxPhi = maxPhi1;
  } else if (same(maxPhi1, minPhi2)) {
    ACTS_VERBOSE("-> CW ordering: one is 'left' of two");
    newMinPhi = minPhi1;
    newMaxPhi = maxPhi2;
    reversed = true;
  } else {
    ACTS_ERROR("Phi ranges are incompatible");
    throw std::invalid_argument("Phi ranges are incompatible");
  }

  ActsScalar newAvgPhi = detail::radian_sym(newMinPhi + newHlPhi);

  ACTS_VERBOSE("merged: [" << newMinPhi / 1_degree << ", "
                           << newMaxPhi / 1_degree << "] ~> "
                           << newAvgPhi / 1_degree << " +- "
                           << newHlPhi / 1_degree);

  return {newHlPhi, newAvgPhi, reversed};
}

}  // namespace Acts::detail
