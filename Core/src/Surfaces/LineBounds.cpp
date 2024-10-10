// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/LineBounds.hpp"

#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"

#include <iomanip>
#include <iostream>

Acts::SurfaceBounds::BoundsType Acts::LineBounds::type() const {
  return SurfaceBounds::eLine;
}

bool Acts::LineBounds::inside(
    const Acts::Vector2& lposition,
    const Acts::BoundaryTolerance& boundaryTolerance) const {
  double r = get(LineBounds::eR);
  double halfLengthZ = get(LineBounds::eHalfLengthZ);
  return detail::insideAlignedBox(Vector2(-r, -halfLengthZ),
                                  Vector2(r, halfLengthZ), boundaryTolerance,
                                  lposition, std::nullopt);
}

// ostream operator overload
std::ostream& Acts::LineBounds::toStream(std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::LineBounds: (radius, halflengthInZ) = ";
  sl << "(" << get(LineBounds::eR) << ", " << get(LineBounds::eHalfLengthZ)
     << ")";
  sl << std::setprecision(-1);
  return sl;
}
