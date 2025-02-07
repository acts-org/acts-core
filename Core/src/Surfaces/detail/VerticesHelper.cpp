// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/detail/VerticesHelper.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace Acts {

std::vector<double> detail::VerticesHelper::phiSegments(
    double phiMin, double phiMax, const std::vector<double>& phiRefs,
    unsigned int quarterSegments) {
  // Check that the phi range is valid
  if (phiMin > phiMax) {
    throw std::invalid_argument(
        "VerticesHelper::phiSegments ... Minimum phi must be smaller than "
        "maximum phi");
  }

  // First check that no reference phi is outside the range
  for (double phiRef : phiRefs) {
    if (phiRef < phiMin || phiRef > phiMax) {
      throw std::invalid_argument(
          "VerticesHelper::phiSegments ... Reference phi is outside the range "
          "of the segment");
    }
  }
  if (quarterSegments == 0u) {
    throw std::invalid_argument(
        "VerticesHelper::phiSegments ... Number of segments must be larger "
        "than 0.");
  }
  std::vector<double> phiSegments = {phiMin, phiMax};
  // Minimum approximation for a circle need
  // - if the circle is closed the last point is given twice
  for (unsigned int i = 0; i < 4 * quarterSegments + 1; ++i) {
    double phiExt =
        -std::numbers::pi + i * 2 * std::numbers::pi / (4 * quarterSegments);
    if (phiExt > phiMin && phiExt < phiMax &&
        std::ranges::none_of(phiSegments, [&phiExt](double phi) {
          return std::abs(phi - phiExt) <
                 std::numeric_limits<double>::epsilon();
        })) {
      phiSegments.push_back(phiExt);
    }
  }
  // Add the reference phis
  for (const auto& phiRef : phiRefs) {
    if (phiRef > phiMin && phiRef < phiMax) {
      if (std::ranges::none_of(phiSegments, [&phiRef](double phi) {
            return std::abs(phi - phiRef) <
                   std::numeric_limits<double>::epsilon();
          })) {
        phiSegments.push_back(phiRef);
      }
    }
  }

  // Sort the phis
  std::ranges::sort(phiSegments);
  return phiSegments;
}

std::vector<Vector2> detail::VerticesHelper::ellipsoidVertices(
    double innerRx, double innerRy, double outerRx, double outerRy,
    double avgPhi, double halfPhi, unsigned int quarterSegments) {
  // List of vertices counter-clockwise starting at smallest phi w.r.t center,
  // for both inner/outer ring/segment
  std::vector<Vector2> rvertices;  // return vertices
  std::vector<Vector2> ivertices;  // inner vertices
  std::vector<Vector2> overtices;  // outer verices

  bool innerExists = (innerRx > 0. && innerRy > 0.);
  bool closed = std::abs(halfPhi - std::numbers::pi) < s_onSurfaceTolerance;

  std::vector<double> refPhi = {};
  if (avgPhi != 0.) {
    refPhi.push_back(avgPhi);
  }

  // The inner (if exists) and outer bow
  if (innerExists) {
    ivertices = segmentVertices<Vector2, Transform2>(
        {innerRx, innerRy}, avgPhi - halfPhi, avgPhi + halfPhi, refPhi,
        quarterSegments);
  }
  overtices = segmentVertices<Vector2, Transform2>(
      {outerRx, outerRy}, avgPhi - halfPhi, avgPhi + halfPhi, refPhi,
      quarterSegments);

  // We want to keep the same counter-clockwise orientation for displaying
  if (!innerExists) {
    if (!closed) {
      // Add the center case we have a sector
      rvertices.push_back(Vector2(0., 0.));
    }
    rvertices.insert(rvertices.end(), overtices.begin(), overtices.end());
  } else if (!closed) {
    rvertices.insert(rvertices.end(), overtices.begin(), overtices.end());
    rvertices.insert(rvertices.end(), ivertices.rbegin(), ivertices.rend());
  } else {
    rvertices.insert(rvertices.end(), overtices.begin(), overtices.end());
    rvertices.insert(rvertices.end(), ivertices.begin(), ivertices.end());
  }
  return rvertices;
}

std::vector<Vector2> detail::VerticesHelper::circularVertices(
    double innerR, double outerR, double avgPhi, double halfPhi,
    unsigned int quarterSegments) {
  return ellipsoidVertices(innerR, innerR, outerR, outerR, avgPhi, halfPhi,
                           quarterSegments);
}

bool detail::VerticesHelper::onHyperPlane(const std::vector<Vector3>& vertices,
                                          double tolerance) {
  // Obvious always on one surface
  if (vertices.size() < 4) {
    return true;
  }
  // Create the hyperplane
  auto hyperPlane = Eigen::Hyperplane<double, 3>::Through(
      vertices[0], vertices[1], vertices[2]);
  for (std::size_t ip = 3; ip < vertices.size(); ++ip) {
    if (hyperPlane.absDistance(vertices[ip]) > tolerance) {
      return false;
    }
  }
  return true;
}

}  // namespace Acts
