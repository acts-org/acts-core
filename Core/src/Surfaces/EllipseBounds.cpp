// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/EllipseBounds.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/VerticesHelper.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <variant>

using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;

Acts::SurfaceBounds::BoundsType Acts::EllipseBounds::type() const {
  return SurfaceBounds::eEllipse;
}

static inline double square(double x) {
  return x * x;
}

/// @warning This **only** works for tolerance-based checks
bool Acts::EllipseBounds::inside(
    const Vector2& lposition,
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
    bool insideInner =
        (get(eInnerRx) <= tol0) || (get(eOuterRx) <= tol0) ||
        (1 < (square(lposition[Acts::eBoundLoc0] / (get(eInnerRx) - tol0)) +
              square(lposition[Acts::eBoundLoc1] / (get(eOuterRx) - tol0))));
    bool insideOuter =
        (square(lposition[Acts::eBoundLoc0] / (get(eInnerRy) + tol0)) +
         square(lposition[Acts::eBoundLoc1] / (get(eOuterRy) + tol0))) < 1;
    return insidePhi && insideInner && insideOuter;
  }

  throw std::logic_error("Unsupported boundary check type");
}

std::vector<Acts::Vector2> Acts::EllipseBounds::vertices(
    unsigned int quarterSegments) const {
  return detail::VerticesHelper::ellipsoidVertices(
      get(eInnerRx), get(eInnerRy), get(eOuterRx), get(eOuterRy),
      get(eAveragePhi), get(eHalfPhiSector), quarterSegments);
}

const Acts::RectangleBounds& Acts::EllipseBounds::boundingBox() const {
  return m_boundingBox;
}

// ostream operator overload
std::ostream& Acts::EllipseBounds::toStream(std::ostream& sl) const {
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
