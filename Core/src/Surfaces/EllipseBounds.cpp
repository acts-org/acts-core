// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/EllipseBounds.hpp"

#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/VerticesHelper.hpp"
#include "Acts/Utilities/MathHelpers.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace Acts {

using VectorHelpers::perp;
using VectorHelpers::phi;

std::vector<double> EllipseBounds::values() const {
  return {m_values.begin(), m_values.end()};
}

void EllipseBounds::checkConsistency() noexcept(false) {
  if (get(eInnerRx) >= get(eOuterRx) || get(eInnerRx) < 0. ||
      get(eOuterRx) <= 0.) {
    throw std::invalid_argument("EllipseBounds: invalid along x axis");
  }
  if (get(eInnerRy) >= get(eOuterRy) || get(eInnerRy) < 0. ||
      get(eOuterRy) <= 0.) {
    throw std::invalid_argument("EllipseBounds: invalid along y axis.");
  }
  if (get(eHalfPhiSector) < 0. || get(eHalfPhiSector) > std::numbers::pi) {
    throw std::invalid_argument("EllipseBounds: invalid phi sector setup.");
  }
  if (get(eAveragePhi) != detail::radian_sym(get(eAveragePhi))) {
    throw std::invalid_argument("EllipseBounds: invalid phi positioning.");
  }
}

bool EllipseBounds::inside(const Vector2& lposition,
                           const BoundaryTolerance& boundaryTolerance) const {
  if (boundaryTolerance.isInfinite()) {
    return true;
  }

  if (auto absoluteBound = boundaryTolerance.asAbsoluteBoundOpt();
      absoluteBound.has_value()) {
    double tol0 = absoluteBound->tolerance0;
    double tol1 = absoluteBound->tolerance1;

    double phi =
        detail::radian_sym(VectorHelpers::phi(lposition) - get(eAveragePhi));
    double phiHalf = get(eHalfPhiSector) + tol1;

    bool insidePhi = (-phiHalf <= phi) && (phi < phiHalf);
    bool insideInner = (get(eInnerRx) <= tol0) || (get(eOuterRx) <= tol0) ||
                       (1 < (square(lposition[0] / (get(eInnerRx) - tol0)) +
                             square(lposition[1] / (get(eOuterRx) - tol0))));
    bool insideOuter = (square(lposition[0] / (get(eInnerRy) + tol0)) +
                        square(lposition[1] / (get(eOuterRy) + tol0))) < 1;
    return insidePhi && insideInner && insideOuter;
  }

  throw std::logic_error("Unsupported boundary check type");
}

std::vector<Vector2> EllipseBounds::vertices(
    unsigned int quarterSegments) const {
  return detail::VerticesHelper::ellipsoidVertices(
      get(eInnerRx), get(eInnerRy), get(eOuterRx), get(eOuterRy),
      get(eAveragePhi), get(eHalfPhiSector), quarterSegments);
}

const RectangleBounds& EllipseBounds::boundingBox() const {
  return m_boundingBox;
}

std::ostream& EllipseBounds::toStream(std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::EllipseBounds:  (innerRadius0, outerRadius0, innerRadius1, "
        "outerRadius1, hPhiSector, averagePhi) = ";
  sl << "(" << get(eInnerRx) << ", " << get(eInnerRy) << ", " << get(eOuterRx)
     << ", " << get(eOuterRy) << ", " << get(eAveragePhi) << ", "
     << get(eHalfPhiSector) << ", " << get(eAveragePhi) << ")";
  sl << std::setprecision(-1);
  return sl;
}

}  // namespace Acts
