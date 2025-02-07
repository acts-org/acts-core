// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/RectangleBounds.hpp"

#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace Acts {

double RectangleBounds::get(BoundValues bValue) const {
  switch (bValue) {
    case eMinX:
      return m_min.x();
    case eMinY:
      return m_min.y();
    case eMaxX:
      return m_max.x();
    case eMaxY:
      return m_max.y();
    default:
      assert(false && "Invalid BoundValue enum value");
      return std::numeric_limits<double>::quiet_NaN();
  }
}

void RectangleBounds::checkConsistency() noexcept(false) {
  if (get(eMinX) > get(eMaxX)) {
    throw std::invalid_argument("RectangleBounds: invalid local x setup");
  }
  if (get(eMinY) > get(eMaxY)) {
    throw std::invalid_argument("RectangleBounds: invalid local y setup");
  }
}

bool RectangleBounds::inside(const Vector2& lposition,
                             const BoundaryTolerance& boundaryTolerance) const {
  return detail::insideAlignedBox(m_min, m_max, boundaryTolerance, lposition,
                                  std::nullopt);
}

std::vector<Vector2> RectangleBounds::vertices(unsigned int /*lseg*/) const {
  // counter-clockwise starting from bottom-left corner
  return {m_min, {m_max.x(), m_min.y()}, m_max, {m_min.x(), m_max.y()}};
}

const RectangleBounds& RectangleBounds::boundingBox() const {
  return (*this);
}

std::ostream& RectangleBounds::toStream(std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::RectangleBounds:  (hlX, hlY) = "
     << "(" << 0.5 * (get(eMaxX) - get(eMinX)) << ", "
     << 0.5 * (get(eMaxY) - get(eMinY)) << ")";
  sl << "\n(lower left, upper right):\n";
  sl << min().transpose() << "\n" << max().transpose();
  sl << std::setprecision(-1);
  return sl;
}

}  // namespace Acts
