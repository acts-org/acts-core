// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/DiscTrapezoidBounds.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <utility>

Acts::DiscTrapezoidBounds::DiscTrapezoidBounds(double halfXminR,
                                               double halfXmaxR, double minR,
                                               double maxR, double avgPhi,
                                               double stereo) noexcept(false)
    : m_values({halfXminR, halfXmaxR, minR, maxR, avgPhi, stereo}) {
  checkConsistency();
  m_ymax = std::sqrt(get(eMaxR) * get(eMaxR) -
                     get(eHalfLengthXmaxR) * get(eHalfLengthXmaxR));
}

Acts::SurfaceBounds::BoundsType Acts::DiscTrapezoidBounds::type() const {
  return SurfaceBounds::eDiscTrapezoid;
}

Acts::Vector2 Acts::DiscTrapezoidBounds::toLocalCartesian(
    const Acts::Vector2& lposition) const {
  return {lposition[eBoundLoc0] *
              std::sin(lposition[eBoundLoc1] - get(eAveragePhi)),
          lposition[eBoundLoc0] *
              std::cos(lposition[eBoundLoc1] - get(eAveragePhi))};
}

Acts::ActsMatrix<2, 2> Acts::DiscTrapezoidBounds::jacobianToLocalCartesian(
    const Acts::Vector2& lposition) const {
  ActsMatrix<2, 2> jacobian;
  jacobian(0, eBoundLoc0) = std::sin(lposition[eBoundLoc1] - get(eAveragePhi));
  jacobian(1, eBoundLoc0) = std::cos(lposition[eBoundLoc1] - get(eAveragePhi));
  jacobian(0, eBoundLoc1) =
      lposition[eBoundLoc0] * std::cos(lposition[eBoundLoc1]);
  jacobian(1, eBoundLoc1) =
      lposition[eBoundLoc0] * -std::sin(lposition[eBoundLoc1]);
  return jacobian;
}

bool Acts::DiscTrapezoidBounds::inside(
    const Acts::Vector2& lposition,
    const Acts::BoundaryTolerance& boundaryTolerance) const {
  Vector2 vertices[] = {{get(eHalfLengthXminR), get(eMinR)},
                        {get(eHalfLengthXmaxR), m_ymax},
                        {-get(eHalfLengthXmaxR), m_ymax},
                        {-get(eHalfLengthXminR), get(eMinR)}};
  auto jacobian = jacobianToLocalCartesian(lposition);
  return detail::insidePolygon(vertices, boundaryTolerance,
                               toLocalCartesian(lposition), jacobian);
}

std::vector<Acts::Vector2> Acts::DiscTrapezoidBounds::vertices(
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

// ostream operator overload
std::ostream& Acts::DiscTrapezoidBounds::toStream(std::ostream& sl) const {
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
