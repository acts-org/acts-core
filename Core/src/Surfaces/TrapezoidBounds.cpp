// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/TrapezoidBounds.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/ConvexPolygonBounds.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"

#include <iomanip>
#include <iostream>

/// Constructor for symmetric Trapezoid
///
/// @param halfXnegY minimal half length X, definition at negative Y
/// @param halfXposY maximal half length X, definition at positive Y
/// @param halfY half length Y - defined at x=0
/// @param rotAngle: rotation angle of the bounds w.r.t coordinate axes
Acts::TrapezoidBounds::TrapezoidBounds(double halfXnegY, double halfXposY,
                                       double halfY,
                                       double rotAngle) noexcept(false)
    : m_values({halfXnegY, halfXposY, halfY, rotAngle}),
      m_boundingBox(std::max(halfXnegY, halfXposY), halfY) {
  rotateBoundingBox();
  checkConsistency();
}

/// Constructor for symmetric Trapezoid - from fixed size array
///
/// @param values the values to be stream in
Acts::TrapezoidBounds::TrapezoidBounds(
    const std::array<double, eSize>& values) noexcept(false)
    : m_values(values),
      m_boundingBox(
          std::max(values[eHalfLengthXnegY], values[eHalfLengthXposY]),
          values[eHalfLengthY]) {
  rotateBoundingBox();
  checkConsistency();
}

Acts::TrapezoidBounds::~TrapezoidBounds() = default;

Acts::SurfaceBounds::BoundsType Acts::TrapezoidBounds::type() const {
  return SurfaceBounds::eTrapezoid;
}

bool Acts::TrapezoidBounds::inside(
    const Acts::Vector2& lposition,
    const Acts::BoundaryTolerance& boundaryTolerance) const {
  if (boundaryTolerance.isInfinite()) {
    return true;
  }

  const double hlXnY = get(TrapezoidBounds::eHalfLengthXnegY);
  const double hlXpY = get(TrapezoidBounds::eHalfLengthXposY);
  const double hlY = get(TrapezoidBounds::eHalfLengthY);
  const double rotAngle = get(TrapezoidBounds::eRotationAngle);

  const Acts::Vector2 extPosition = Eigen::Rotation2Dd(rotAngle) * lposition;
  const double x = extPosition[0];
  const double y = extPosition[1];

  if (auto absoluteBound = boundaryTolerance.asAbsoluteBoundOpt(true);
      absoluteBound.has_value()) {
    double tolX = absoluteBound->tolerance0;
    double tolY = absoluteBound->tolerance1;

    if (std::abs(y) - hlY > tolY) {
      // outside y range
      return false;
    }

    if (std::abs(x) - std::max(hlXnY, hlXpY) > tolX) {
      // outside x range
      return false;
    }

    if (std::abs(x) - std::min(hlXnY, hlXpY) <= tolX) {
      // inside x range
      return true;
    }
  }

  // at this stage, the point can only be in the triangles
  // run slow-ish polygon check
  Vector2 vertices[] = {
      {-hlXnY, -hlY}, {hlXnY, -hlY}, {hlXpY, hlY}, {-hlXpY, hlY}};
  return detail::insidePolygon(vertices, boundaryTolerance, extPosition,
                               std::nullopt);
}

std::vector<Acts::Vector2> Acts::TrapezoidBounds::vertices(
    unsigned int /*ignoredSegments*/) const {
  const double hlXnY = get(TrapezoidBounds::eHalfLengthXnegY);
  const double hlXpY = get(TrapezoidBounds::eHalfLengthXposY);
  const double hlY = get(TrapezoidBounds::eHalfLengthY);
  const double rotAngle = get(TrapezoidBounds::eRotationAngle);

  std::vector<Acts::Vector2> vertices = {
      {-hlXnY, -hlY}, {hlXnY, -hlY}, {hlXpY, hlY}, {-hlXpY, hlY}};
  for (auto& v : vertices) {
    v = Eigen::Rotation2Dd(-rotAngle) * v;
  }
  return vertices;
}

const Acts::RectangleBounds& Acts::TrapezoidBounds::boundingBox() const {
  return m_boundingBox;
}

std::ostream& Acts::TrapezoidBounds::toStream(std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::TrapezoidBounds:  (halfXnegY, halfXposY, halfY, rotAngle) = "
     << "(" << get(eHalfLengthXnegY) << ", " << get(eHalfLengthXposY) << ", "
     << get(eHalfLengthY) << ", " << get(eRotationAngle) << ")";
  sl << std::setprecision(-1);
  return sl;
}

std::vector<double> Acts::TrapezoidBounds::values() const {
  std::vector<double> valvector;
  valvector.insert(valvector.begin(), m_values.begin(), m_values.end());
  return valvector;
}

void Acts::TrapezoidBounds::rotateBoundingBox() noexcept(false) {
  const double rotAngle = get(eRotationAngle);

  if (rotAngle != 0.) {
    m_boundingBox = ConvexPolygonBounds<4>(vertices()).boundingBox();
  }
}

void Acts::TrapezoidBounds::checkConsistency() noexcept(false) {
  if (get(eHalfLengthXnegY) <= 0. || get(eHalfLengthXposY) <= 0.) {
    throw std::invalid_argument("TrapezoidBounds: invalid local x setup");
  }
  if (get(eHalfLengthY) <= 0.) {
    throw std::invalid_argument("TrapezoidBounds: invalid local y setup");
  }
}
