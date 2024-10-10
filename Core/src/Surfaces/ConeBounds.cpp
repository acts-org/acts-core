// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/ConeBounds.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>

Acts::ConeBounds::ConeBounds(double alpha, bool symm, double halfphi,
                             double avphi) noexcept(false)
    : m_values({alpha, symm ? -std::numeric_limits<double>::infinity() : 0,
                std::numeric_limits<double>::infinity(), halfphi, avphi}),
      m_tanAlpha(std::tan(alpha)) {
  checkConsistency();
}

Acts::ConeBounds::ConeBounds(double alpha, double minz, double maxz,
                             double halfphi, double avphi) noexcept(false)
    : m_values({alpha, minz, maxz, halfphi, avphi}),
      m_tanAlpha(std::tan(alpha)) {
  checkConsistency();
}

Acts::ConeBounds::ConeBounds(const std::array<double, eSize>& values) noexcept(
    false)
    : m_values(values), m_tanAlpha(std::tan(values[eAlpha])) {
  checkConsistency();
}

Acts::SurfaceBounds::BoundsType Acts::ConeBounds::type() const {
  return SurfaceBounds::eCone;
}

/// Shift r-phi coordinate to be centered around the average phi.
Acts::Vector2 Acts::ConeBounds::shifted(const Acts::Vector2& lposition) const {
  using Acts::detail::radian_sym;

  auto x = r(lposition[eBoundLoc1]);  // cone radius at the local position
  Vector2 shifted;
  shifted[eBoundLoc1] = lposition[eBoundLoc1];
  shifted[eBoundLoc0] =
      std::isnormal(x)
          ? (x * radian_sym((lposition[eBoundLoc0] / x) - get(eAveragePhi)))
          : lposition[eBoundLoc0];
  return shifted;
}

bool Acts::ConeBounds::inside(
    const Acts::Vector2& lposition,
    const Acts::BoundaryTolerance& boundaryTolerance) const {
  auto rphiHalf = r(lposition[eBoundLoc1]) * get(eHalfPhiSector);
  return detail::insideAlignedBox(
      Vector2(-rphiHalf, get(eMinZ)), Vector2(rphiHalf, get(eMaxZ)),
      boundaryTolerance, shifted(lposition), std::nullopt);
}

std::ostream& Acts::ConeBounds::toStream(std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::ConeBounds: (tanAlpha, minZ, maxZ, halfPhiSector, averagePhi) "
        "= ";
  sl << "(" << m_tanAlpha << ", " << get(eMinZ) << ", " << get(eMaxZ) << ", "
     << get(eHalfPhiSector) << ", " << get(eAveragePhi) << ")";
  sl << std::setprecision(-1);
  return sl;
}
