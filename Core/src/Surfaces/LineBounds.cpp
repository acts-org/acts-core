// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/LineBounds.hpp"

#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"

#include <iomanip>
#include <iostream>

namespace Acts {

std::vector<double> LineBounds::values() const {
  return {m_values.begin(), m_values.end()};
}

void LineBounds::checkConsistency() noexcept(false) {
  if (get(eR) < 0.) {
    throw std::invalid_argument("LineBounds: zero radius.");
  }
  if (get(eHalfLengthZ) <= 0.) {
    throw std::invalid_argument("LineBounds: zero/negative length.");
  }
}

bool LineBounds::inside(const Vector2& lposition,
                        const BoundaryTolerance& boundaryTolerance) const {
  double r = get(LineBounds::eR);
  double halfLengthZ = get(LineBounds::eHalfLengthZ);
  return detail::insideAlignedBox(Vector2(-r, -halfLengthZ),
                                  Vector2(r, halfLengthZ), boundaryTolerance,
                                  lposition, std::nullopt);
}

std::ostream& LineBounds::toStream(std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::LineBounds: (radius, halflengthInZ) = ";
  sl << "(" << get(LineBounds::eR) << ", " << get(LineBounds::eHalfLengthZ)
     << ")";
  sl << std::setprecision(-1);
  return sl;
}

}  // namespace Acts
