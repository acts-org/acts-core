// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/detail/IntersectionHelper2D.hpp"

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "Acts/Utilities/detail/RealQuadraticEquation.hpp"

#include <cmath>

Acts::Intersection2D Acts::detail::IntersectionHelper2D::intersectSegment(
    const Vector2& s0, const Vector2& s1, const Vector2& origin,
    const Vector2& dir, bool boundCheck) {
  using Line = Eigen::ParametrizedLine<ActsScalar, 2>;
  using Plane = Eigen::Hyperplane<ActsScalar, 2>;

  Vector2 edge(s1 - s0);
  ActsScalar det = edge.x() * dir.y() - edge.y() * dir.x();
  if (std::abs(det) < s_epsilon) {
    return Intersection2D::invalid();
  }

  auto line = Line(origin, dir);
  auto d = line.intersectionParameter(Plane::Through(s0, s1));

  Vector2 intersection(origin + d * dir);
  Intersection2D::Status status = Intersection2D::Status::reachable;
  if (boundCheck) {
    auto edgeToSol = intersection - s0;
    if (edgeToSol.dot(edge) < 0. || edgeToSol.norm() > (edge).norm()) {
      status = Intersection2D::Status::unreachable;
    }
  }
  return Intersection2D(intersection, d, status);
}

std::array<Acts::Intersection2D, 2>
Acts::detail::IntersectionHelper2D::intersectEllipse(ActsScalar Rx,
                                                     ActsScalar Ry,
                                                     const Vector2& origin,
                                                     const Vector2& dir) {
  auto createSolution =
      [&](const Vector2& sol,
          const Vector2& alt) -> std::array<Acts::Intersection2D, 2> {
    Vector2 toSolD(sol - origin);
    Vector2 toAltD(alt - origin);

    ActsScalar solD = std::copysign(toSolD.norm(), toSolD.dot(dir));
    ActsScalar altD = std::copysign(toAltD.norm(), toAltD.dot(dir));

    if (std::abs(solD) < std::abs(altD)) {
      return {Intersection2D(sol, solD, Intersection2D::Status::reachable),
              Intersection2D(alt, altD, Intersection2D::Status::reachable)};
    }
    return {Intersection2D(alt, altD, Intersection2D::Status::reachable),
            Intersection2D(sol, solD, Intersection2D::Status::reachable)};
  };

  // Special cases first
  if (std::abs(dir.x()) < s_epsilon) {
    ActsScalar solx = origin.x();
    ActsScalar D = 1. - solx * solx / (Rx * Rx);
    if (D > 0.) {
      ActsScalar sqrtD = std::sqrt(D);
      Vector2 sol(solx, Ry * sqrtD);
      Vector2 alt(solx, -Ry * sqrtD);
      return createSolution(sol, alt);
    } else if (std::abs(D) < s_epsilon) {
      return {Intersection2D(Vector2(solx, 0.), -origin.y(),
                             Intersection2D::Status::reachable),
              Intersection2D::invalid()};
    }
    return {Intersection2D::invalid(), Intersection2D::invalid()};
  } else if (std::abs(dir.y()) < s_epsilon) {
    ActsScalar soly = origin.y();
    ActsScalar D = 1. - soly * soly / (Ry * Ry);
    if (D > 0.) {
      ActsScalar sqrtD = std::sqrt(D);
      Vector2 sol(Rx * sqrtD, soly);
      Vector2 alt(-Rx * sqrtD, soly);
      return createSolution(sol, alt);
    } else if (std::abs(D) < s_epsilon) {
      return {Intersection2D(Vector2(0., soly), -origin.x(),
                             Intersection2D::Status::reachable),
              Intersection2D::invalid()};
    }
    return {Intersection2D::invalid(), Intersection2D::invalid()};
  }
  // General solution
  ActsScalar k = dir.y() / dir.x();
  ActsScalar d = origin.y() - k * origin.x();
  ActsScalar Ry2 = Ry * Ry;
  ActsScalar alpha = 1. / (Rx * Rx) + k * k / Ry2;
  ActsScalar beta = 2. * k * d / Ry2;
  ActsScalar gamma = d * d / Ry2 - 1;
  Acts::detail::RealQuadraticEquation solver(alpha, beta, gamma);
  if (solver.solutions == 1) {
    ActsScalar x = solver.first;
    Vector2 sol(x, k * x + d);
    Vector2 toSolD(sol - origin);
    ActsScalar solD = std::copysign(toSolD.norm(), toSolD.dot(dir));
    return {Intersection2D(sol, solD, Intersection2D::Status::reachable),
            Intersection2D::invalid()};
  } else if (solver.solutions > 1) {
    ActsScalar x0 = solver.first;
    ActsScalar x1 = solver.second;
    Vector2 sol(x0, k * x0 + d);
    Vector2 alt(x1, k * x1 + d);
    return createSolution(sol, alt);
  }
  return {Intersection2D::invalid(), Intersection2D::invalid()};
}

Acts::Intersection2D Acts::detail::IntersectionHelper2D::intersectCircleSegment(
    ActsScalar R, ActsScalar phiMin, ActsScalar phiMax, const Vector2& origin,
    const Vector2& dir) {
  auto intersections = intersectCircle(R, origin, dir);
  for (const auto& candidate : intersections) {
    if (candidate.pathLength() > 0.) {
      ActsScalar phi = Acts::VectorHelpers::phi(candidate.position());
      if (phi > phiMin && phi < phiMax) {
        return candidate;
      }
    }
  }
  return Intersection2D::invalid();
}
