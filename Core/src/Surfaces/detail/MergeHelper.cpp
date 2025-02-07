// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/detail/MergeHelper.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <numbers>

namespace Acts::detail {

std::tuple<double, double, bool> mergedPhiSector(double hlPhi1, double avgPhi1,
                                                 double hlPhi2, double avgPhi2,
                                                 const Logger& logger,
                                                 double tolerance) {
  using namespace UnitLiterals;

  if (std::abs(hlPhi1 - std::numbers::pi / 2.) < tolerance &&
      std::abs(hlPhi2 - std::numbers::pi / 2.) < tolerance) {
    ACTS_VERBOSE("Both phi sectors cover a half circle");

    ACTS_VERBOSE("-> distance between sectors: " << detail::difference_periodic(
                                                        avgPhi1, avgPhi2,
                                                        2 * std::numbers::pi) /
                                                        1_degree);

    if (std::abs(std::abs(detail::difference_periodic(avgPhi1, avgPhi2,
                                                      2 * std::numbers::pi)) -
                 std::numbers::pi) > tolerance) {
      throw std::invalid_argument(
          "Phi sectors cover half a circle but are not opposite");
    }

    double newAvgPhi = detail::radian_sym(avgPhi1 + std::numbers::pi / 2.);
    double newHlPhi = std::numbers::pi;
    ACTS_VERBOSE("merged: ["
                 << detail::radian_sym(newAvgPhi - newHlPhi) / 1_degree << ", "
                 << detail::radian_sym(newAvgPhi + newHlPhi) / 1_degree
                 << "] ~> " << newAvgPhi / 1_degree << " +- "
                 << newHlPhi / 1_degree);
    return {newHlPhi, newAvgPhi, false};
  }

  double minPhi1 = detail::radian_sym(-hlPhi1 + avgPhi1);
  double maxPhi1 = detail::radian_sym(hlPhi1 + avgPhi1);

  ACTS_VERBOSE("one: [" << minPhi1 / 1_degree << ", " << maxPhi1 / 1_degree
                        << "] ~> " << avgPhi1 / 1_degree << " +- "
                        << hlPhi1 / 1_degree);

  double maxPhi2 = detail::radian_sym(hlPhi2 + avgPhi2);
  double minPhi2 = detail::radian_sym(-hlPhi2 + avgPhi2);

  ACTS_VERBOSE("two: [" << minPhi2 / 1_degree << ", " << maxPhi2 / 1_degree
                        << "] ~> " << avgPhi2 / 1_degree << " +- "
                        << hlPhi2 / 1_degree);

  ACTS_VERBOSE("Checking for CCW or CW ordering");
  auto same = [tolerance](double a, double b) {
    return std::abs(a - b) < tolerance;
  };

  double newMaxPhi{}, newMinPhi{};
  double newHlPhi = hlPhi1 + hlPhi2;

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

  double newAvgPhi = detail::radian_sym(newMinPhi + newHlPhi);

  ACTS_VERBOSE("merged: [" << newMinPhi / 1_degree << ", "
                           << newMaxPhi / 1_degree << "] ~> "
                           << newAvgPhi / 1_degree << " +- "
                           << newHlPhi / 1_degree);

  return {newHlPhi, newAvgPhi, reversed};
}

}  // namespace Acts::detail
