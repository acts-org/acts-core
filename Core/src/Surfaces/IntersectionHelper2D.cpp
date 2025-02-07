// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/detail/IntersectionHelper2D.hpp"

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "Acts/Utilities/detail/RealQuadraticEquation.hpp"

#include <cmath>

namespace Acts::detail {

Intersection2D IntersectionHelper2D::intersectSegment(const Vector2& s0,
                                                      const Vector2& s1,
                                                      const Vector2& origin,
                                                      const Vector2& dir,
                                                      bool boundCheck) {
  using Line = Eigen::ParametrizedLine<double, 2>;
  using Plane = Eigen::Hyperplane<double, 2>;

  Vector2 edge(s1 - s0);
  double det = edge.x() * dir.y() - edge.y() * dir.x();
  if (std::abs(det) < s_epsilon) {
    return Intersection2D::invalid();
  }

  auto line = Line(origin, dir);
  auto d = line.intersectionParameter(Plane::Through(s0, s1));

  Vector2 intersection(origin + d * dir);
  IntersectionStatus status = IntersectionStatus::reachable;
  if (boundCheck) {
    auto edgeToSol = intersection - s0;
    if (edgeToSol.dot(edge) < 0. || edgeToSol.norm() > (edge).norm()) {
      status = IntersectionStatus::unreachable;
    }
  }
  return Intersection2D(intersection, d, status);
}

std::array<Intersection2D, 2> IntersectionHelper2D::intersectEllipse(
    double Rx, double Ry, const Vector2& origin, const Vector2& dir) {
  auto createSolution =
      [&](const Vector2& sol,
          const Vector2& alt) -> std::array<Intersection2D, 2> {
    Vector2 toSolD(sol - origin);
    Vector2 toAltD(alt - origin);

    double solD = std::copysign(toSolD.norm(), toSolD.dot(dir));
    double altD = std::copysign(toAltD.norm(), toAltD.dot(dir));

    if (std::abs(solD) < std::abs(altD)) {
      return {Intersection2D(sol, solD, IntersectionStatus::reachable),
              Intersection2D(alt, altD, IntersectionStatus::reachable)};
    }
    return {Intersection2D(alt, altD, IntersectionStatus::reachable),
            Intersection2D(sol, solD, IntersectionStatus::reachable)};
  };

  // Special cases first
  if (std::abs(dir.x()) < s_epsilon) {
    double solx = origin.x();
    double D = 1. - solx * solx / (Rx * Rx);
    if (D > 0.) {
      double sqrtD = std::sqrt(D);
      Vector2 sol(solx, Ry * sqrtD);
      Vector2 alt(solx, -Ry * sqrtD);
      return createSolution(sol, alt);
    } else if (std::abs(D) < s_epsilon) {
      return {Intersection2D(Vector2(solx, 0.), -origin.y(),
                             IntersectionStatus::reachable),
              Intersection2D::invalid()};
    }
    return {Intersection2D::invalid(), Intersection2D::invalid()};
  } else if (std::abs(dir.y()) < s_epsilon) {
    double soly = origin.y();
    double D = 1. - soly * soly / (Ry * Ry);
    if (D > 0.) {
      double sqrtD = std::sqrt(D);
      Vector2 sol(Rx * sqrtD, soly);
      Vector2 alt(-Rx * sqrtD, soly);
      return createSolution(sol, alt);
    } else if (std::abs(D) < s_epsilon) {
      return {Intersection2D(Vector2(0., soly), -origin.x(),
                             IntersectionStatus::reachable),
              Intersection2D::invalid()};
    }
    return {Intersection2D::invalid(), Intersection2D::invalid()};
  }
  // General solution
  double k = dir.y() / dir.x();
  double d = origin.y() - k * origin.x();
  double Ry2 = Ry * Ry;
  double alpha = 1. / (Rx * Rx) + k * k / Ry2;
  double beta = 2. * k * d / Ry2;
  double gamma = d * d / Ry2 - 1;
  RealQuadraticEquation solver(alpha, beta, gamma);
  if (solver.solutions == 1) {
    double x = solver.first;
    Vector2 sol(x, k * x + d);
    Vector2 toSolD(sol - origin);
    double solD = std::copysign(toSolD.norm(), toSolD.dot(dir));
    return {Intersection2D(sol, solD, IntersectionStatus::reachable),
            Intersection2D::invalid()};
  } else if (solver.solutions > 1) {
    double x0 = solver.first;
    double x1 = solver.second;
    Vector2 sol(x0, k * x0 + d);
    Vector2 alt(x1, k * x1 + d);
    return createSolution(sol, alt);
  }
  return {Intersection2D::invalid(), Intersection2D::invalid()};
}

Intersection2D IntersectionHelper2D::intersectCircleSegment(
    double R, double phiMin, double phiMax, const Vector2& origin,
    const Vector2& dir) {
  auto intersections = intersectCircle(R, origin, dir);
  for (const auto& candidate : intersections) {
    if (candidate.pathLength() > 0.) {
      double phi = VectorHelpers::phi(candidate.position());
      if (phi > phiMin && phi < phiMax) {
        return candidate;
      }
    }
  }
  return Intersection2D::invalid();
}

}  // namespace Acts::detail
