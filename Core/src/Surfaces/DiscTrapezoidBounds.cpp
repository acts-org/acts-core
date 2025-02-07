// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/DiscTrapezoidBounds.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace Acts {

DiscTrapezoidBounds::DiscTrapezoidBounds(double halfXminR, double halfXmaxR,
                                         double minR, double maxR,
                                         double avgPhi,
                                         double stereo) noexcept(false)
    : m_values({halfXminR, halfXmaxR, minR, maxR, avgPhi, stereo}) {
  checkConsistency();
  m_ymax = std::sqrt(get(eMaxR) * get(eMaxR) -
                     get(eHalfLengthXmaxR) * get(eHalfLengthXmaxR));
}

std::vector<double> DiscTrapezoidBounds::values() const {
  return {m_values.begin(), m_values.end()};
}

void DiscTrapezoidBounds::checkConsistency() noexcept(false) {
  if (get(eMinR) < 0. || get(eMaxR) <= 0. || get(eMinR) > get(eMaxR)) {
    throw std::invalid_argument("DiscTrapezoidBounds: invalid radial setup.");
  }
  if (get(eHalfLengthXminR) < 0. || get(eHalfLengthXmaxR) <= 0.) {
    throw std::invalid_argument("DiscTrapezoidBounds: negative length given.");
  }
  if (get(eAveragePhi) != detail::radian_sym(get(eAveragePhi))) {
    throw std::invalid_argument(
        "DiscTrapezoidBounds: invalid phi positioning.");
  }
}

Vector2 DiscTrapezoidBounds::toLocalCartesian(const Vector2& lposition) const {
  return {lposition[0] * std::sin(lposition[1] - get(eAveragePhi)),
          lposition[0] * std::cos(lposition[1] - get(eAveragePhi))};
}

SquareMatrix2 DiscTrapezoidBounds::jacobianToLocalCartesian(
    const Vector2& lposition) const {
  SquareMatrix2 jacobian;
  jacobian(0, 0) = std::sin(lposition[1] - get(eAveragePhi));
  jacobian(1, 0) = std::cos(lposition[1] - get(eAveragePhi));
  jacobian(0, 1) = lposition[0] * std::cos(lposition[1]);
  jacobian(1, 1) = lposition[0] * -std::sin(lposition[1]);
  return jacobian;
}

bool DiscTrapezoidBounds::inside(
    const Vector2& lposition,
    const BoundaryTolerance& boundaryTolerance) const {
  Vector2 vertices[] = {{get(eHalfLengthXminR), get(eMinR)},
                        {get(eHalfLengthXmaxR), m_ymax},
                        {-get(eHalfLengthXmaxR), m_ymax},
                        {-get(eHalfLengthXminR), get(eMinR)}};
  auto jacobian = jacobianToLocalCartesian(lposition);
  return detail::insidePolygon(vertices, boundaryTolerance,
                               toLocalCartesian(lposition), jacobian);
}

std::vector<Vector2> DiscTrapezoidBounds::vertices(
    unsigned int /*ignoredSegments*/) const {
  Vector2 cAxis(std::cos(get(eAveragePhi)), std::sin(get(eAveragePhi)));
  Vector2 nAxis(cAxis.y(), -cAxis.x());
  auto ymin = std::sqrt(get(eMinR) * get(eMinR) -
                        get(eHalfLengthXminR) * get(eHalfLengthXminR));
  auto halfY = (m_ymax - ymin) / 2;
  return {-halfY * cAxis - get(eHalfLengthXminR) * nAxis,
          -halfY * cAxis + get(eHalfLengthXminR) * nAxis,
          halfY * cAxis + get(eHalfLengthXmaxR) * nAxis,
          halfY * cAxis - get(eHalfLengthXmaxR) * nAxis};
}

std::ostream& DiscTrapezoidBounds::toStream(std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::DiscTrapezoidBounds: (innerRadius, outerRadius, "
        "halfLengthXminR, "
        "halfLengthXmaxR, halfLengthY, halfPhiSector, averagePhi, rCenter, "
        "stereo) = ";
  sl << "(" << get(eMinR) << ", " << get(eMaxR) << ", " << get(eHalfLengthXminR)
     << ", " << get(eHalfLengthXmaxR) << ", " << halfLengthY() << ", "
     << halfPhiSector() << ", " << get(eAveragePhi) << ", " << rCenter() << ", "
     << stereo() << ")";
  sl << std::setprecision(-1);
  return sl;
}

}  // namespace Acts
