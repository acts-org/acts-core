// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cmath>
#include <iostream>
#include <tuple>

#include "Acts/Surfaces/detail/IntersectionHelper2D.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/detail/RealQuadraticEquation.hpp"

Acts::Intersection2D Acts::detail::IntersectionHelper2D::intersectSegment(
    const Vector2D& s0, const Vector2D& s1, const Vector2D& origin,
    const Vector2D& dir, bool boundCheck) {
  using Line = Eigen::ParametrizedLine<AlgebraScalar, 2>;
  using Plane = Eigen::Hyperplane<AlgebraScalar, 2>;

  Vector2D edge(s1 - s0);
  AlgebraScalar det = edge.x() * dir.y() - edge.y() * dir.x();
  if (std::abs(det) < s_epsilon) {
    return Intersection2D();
  }

  auto line = Line(origin, dir);
  auto d = line.intersectionParameter(Plane::Through(s0, s1));

  Vector2D intersection(origin + d * dir);
  Intersection2D::Status status = Intersection2D::Status::reachable;
  if (boundCheck) {
    auto edgeToSol = intersection - s0;
    if (edgeToSol.dot(edge) < 0. or edgeToSol.norm() > (edge).norm()) {
      status = Intersection2D::Status::unreachable;
    }
  }
  return Intersection2D(intersection, d, status);
}

std::array<Acts::Intersection2D, 2>
Acts::detail::IntersectionHelper2D::intersectEllipse(AlgebraScalar Rx,
                                                     AlgebraScalar Ry,
                                                     const Vector2D& origin,
                                                     const Vector2D& dir) {
  auto createSolution =
      [&](const Vector2D& sol,
          const Vector2D& alt) -> std::array<Acts::Intersection2D, 2> {
    Vector2D toSolD(sol - origin);
    Vector2D toAltD(alt - origin);

    AlgebraScalar solD = std::copysign(toSolD.norm(), toSolD.dot(dir));
    AlgebraScalar altD = std::copysign(toAltD.norm(), toAltD.dot(dir));

    if (solD * solD < altD * altD) {
      return {Intersection2D(sol, solD, Intersection2D::Status::reachable),
              Intersection2D(alt, altD, Intersection2D::Status::reachable)};
    }
    return {Intersection2D(alt, altD, Intersection2D::Status::reachable),
            Intersection2D(sol, solD, Intersection2D::Status::reachable)};
  };

  // Special cases first
  if (std::abs(dir.x()) < s_epsilon) {
    AlgebraScalar solx = origin.x();
    AlgebraScalar D = 1. - solx * solx / (Rx * Rx);
    if (D > 0.) {
      AlgebraScalar sqrtD = std::sqrt(D);
      Vector2D sol(solx, Ry * sqrtD);
      Vector2D alt(solx, -Ry * sqrtD);
      return createSolution(sol, alt);
    } else if (std::abs(D) < s_epsilon) {
      return {Intersection2D(Vector2D(solx, 0.), -origin.y(),
                             Intersection2D::Status::reachable),
              Intersection2D()};
    }
    return {Intersection2D(), Intersection2D()};
  } else if (std::abs(dir.y()) < s_epsilon) {
    AlgebraScalar soly = origin.y();
    AlgebraScalar D = 1. - soly * soly / (Ry * Ry);
    if (D > 0.) {
      AlgebraScalar sqrtD = std::sqrt(D);
      Vector2D sol(Rx * sqrtD, soly);
      Vector2D alt(-Rx * sqrtD, soly);
      return createSolution(sol, alt);
    } else if (std::abs(D) < s_epsilon) {
      return {Intersection2D(Vector2D(0., soly), -origin.x(),
                             Intersection2D::Status::reachable),
              Intersection2D()};
    }
    return {Intersection2D(), Intersection2D()};
  }
  // General solution
  AlgebraScalar k = dir.y() / dir.x();
  AlgebraScalar d = origin.y() - k * origin.x();
  AlgebraScalar Ry2 = Ry * Ry;
  AlgebraScalar alpha = 1. / (Rx * Rx) + k * k / Ry2;
  AlgebraScalar beta = 2. * k * d / Ry2;
  AlgebraScalar gamma = d * d / Ry2 - 1;
  Acts::detail::RealQuadraticEquation solver(alpha, beta, gamma);
  if (solver.solutions == 1) {
    AlgebraScalar x = solver.first;
    Vector2D sol(x, k * x + d);
    Vector2D toSolD(sol - origin);
    AlgebraScalar solD = std::copysign(toSolD.norm(), toSolD.dot(dir));
    return {Intersection2D(sol, solD, Intersection2D::Status::reachable),
            Intersection2D()};
  } else if (solver.solutions > 1) {
    AlgebraScalar x0 = solver.first;
    AlgebraScalar x1 = solver.second;
    Vector2D sol(x0, k * x0 + d);
    Vector2D alt(x1, k * x1 + d);
    return createSolution(sol, alt);
  }
  return {Intersection2D(), Intersection2D()};
}

Acts::Intersection2D Acts::detail::IntersectionHelper2D::intersectCircleSegment(
    AlgebraScalar R, AlgebraScalar phiMin, AlgebraScalar phiMax,
    const Vector2D& origin, const Vector2D& dir) {
  auto intersections = intersectCircle(R, origin, dir);
  for (const auto& candidate : intersections) {
    if (candidate.pathLength > 0.) {
      AlgebraScalar phi = Acts::VectorHelpers::phi(candidate.position);
      if (phi > phiMin and phi < phiMax) {
        return candidate;
      }
    }
  }
  return Intersection2D();
}
