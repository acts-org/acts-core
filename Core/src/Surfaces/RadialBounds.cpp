// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/RadialBounds.hpp"

#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"
#include "Acts/Surfaces/detail/VerticesHelper.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace Acts {

std::vector<double> RadialBounds::values() const {
  return {m_values.begin(), m_values.end()};
}

void RadialBounds::checkConsistency() noexcept(false) {
  if (get(eMinR) < 0. || get(eMaxR) <= 0. || get(eMinR) > get(eMaxR)) {
    throw std::invalid_argument("RadialBounds: invalid radial setup");
  }
  if (get(eHalfPhiSector) < 0. || get(eHalfPhiSector) > std::numbers::pi) {
    throw std::invalid_argument("RadialBounds: invalid phi sector setup.");
  }
  if (get(eAveragePhi) != detail::radian_sym(get(eAveragePhi))) {
    throw std::invalid_argument("RadialBounds: invalid phi positioning.");
  }
}

Vector2 RadialBounds::shifted(const Vector2& lposition) const {
  Vector2 tmp;
  tmp[0] = lposition[0];
  tmp[1] = detail::radian_sym(lposition[1] - get(eAveragePhi));
  return tmp;
}

bool RadialBounds::inside(const Vector2& lposition,
                          const BoundaryTolerance& boundaryTolerance) const {
  return detail::insideAlignedBox(Vector2(get(eMinR), -get(eHalfPhiSector)),
                                  Vector2(get(eMaxR), get(eHalfPhiSector)),
                                  boundaryTolerance, shifted(lposition),
                                  std::nullopt);
}

std::vector<Vector2> RadialBounds::vertices(unsigned int lseg) const {
  return detail::VerticesHelper::circularVertices(
      get(eMinR), get(eMaxR), get(eAveragePhi), get(eHalfPhiSector), lseg);
}

std::ostream& RadialBounds::toStream(std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::RadialBounds:  (innerRadius, outerRadius, hPhiSector, "
        "averagePhi) = ";
  sl << "(" << get(eMinR) << ", " << get(eMaxR) << ", " << get(eHalfPhiSector)
     << ", " << get(eAveragePhi) << ")";
  sl << std::setprecision(-1);
  return sl;
}

}  // namespace Acts
