// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/ConeBounds.hpp"

#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>

namespace Acts {

ConeBounds::ConeBounds(double alpha, bool symm, double halfphi,
                       double avphi) noexcept(false)
    : m_values({alpha, symm ? -std::numeric_limits<double>::infinity() : 0,
                std::numeric_limits<double>::infinity(), halfphi, avphi}),
      m_tanAlpha(std::tan(alpha)) {
  checkConsistency();
}

ConeBounds::ConeBounds(double alpha, double minz, double maxz, double halfphi,
                       double avphi) noexcept(false)
    : m_values({alpha, minz, maxz, halfphi, avphi}),
      m_tanAlpha(std::tan(alpha)) {
  checkConsistency();
}

ConeBounds::ConeBounds(const std::array<double, eSize>& values) noexcept(false)
    : m_values(values), m_tanAlpha(std::tan(values[eAlpha])) {
  checkConsistency();
}

std::vector<double> ConeBounds::values() const {
  return {m_values.begin(), m_values.end()};
}

void ConeBounds::checkConsistency() noexcept(false) {
  if (get(eAlpha) < 0. || get(eAlpha) >= std::numbers::pi) {
    throw std::invalid_argument("ConeBounds: invalid open angle.");
  }
  if (get(eMinZ) > get(eMaxZ) ||
      std::abs(get(eMinZ) - get(eMaxZ)) < s_epsilon) {
    throw std::invalid_argument("ConeBounds: invalid z range setup.");
  }
  if (get(eHalfPhiSector) < 0. || std::abs(eHalfPhiSector) > std::numbers::pi) {
    throw std::invalid_argument("ConeBounds: invalid phi sector setup.");
  }
  if (get(eAveragePhi) != detail::radian_sym(get(eAveragePhi))) {
    throw std::invalid_argument("ConeBounds: invalid phi positioning.");
  }
}

Vector2 ConeBounds::shifted(const Vector2& lposition) const {
  using detail::radian_sym;

  auto x = r(lposition[1]);  // cone radius at the local position
  Vector2 shifted;
  shifted[1] = lposition[1];
  shifted[0] = std::isnormal(x)
                   ? (x * radian_sym((lposition[0] / x) - get(eAveragePhi)))
                   : lposition[0];
  return shifted;
}

bool ConeBounds::inside(const Vector2& lposition,
                        const BoundaryTolerance& boundaryTolerance) const {
  auto rphiHalf = r(lposition[1]) * get(eHalfPhiSector);
  return detail::insideAlignedBox(
      Vector2(-rphiHalf, get(eMinZ)), Vector2(rphiHalf, get(eMaxZ)),
      boundaryTolerance, shifted(lposition), std::nullopt);
}

std::ostream& ConeBounds::toStream(std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::ConeBounds: (tanAlpha, minZ, maxZ, halfPhiSector, averagePhi) "
        "= ";
  sl << "(" << m_tanAlpha << ", " << get(eMinZ) << ", " << get(eMaxZ) << ", "
     << get(eHalfPhiSector) << ", " << get(eAveragePhi) << ")";
  sl << std::setprecision(-1);
  return sl;
}

}  // namespace Acts
