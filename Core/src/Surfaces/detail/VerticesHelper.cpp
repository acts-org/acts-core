// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/detail/VerticesHelper.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

std::vector<Acts::ActsScalar> Acts::detail::VerticesHelper::phiSegments(
    ActsScalar phiMin, ActsScalar phiMax,
    const std::vector<ActsScalar>& phiRefs, unsigned int quarterSegments) {
  // Check that the phi range is valid
  if (phiMin > phiMax) {
    throw std::invalid_argument(
        "VerticesHelper::phiSegments ... Minimum phi must be smaller than "
        "maximum phi");
  }

  // First check that no reference phi is outside the range
  for (ActsScalar phiRef : phiRefs) {
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
  std::vector<ActsScalar> phiSegments = {phiMin, phiMax};
  // Minimum approximation for a circle need
  // - if the circle is closed the last point is given twice
  for (unsigned int i = 0; i < 4 * quarterSegments + 1; ++i) {
    ActsScalar phiExt = -M_PI + i * 2 * M_PI / (4 * quarterSegments);
    if (phiExt > phiMin && phiExt < phiMax &&
        std::ranges::none_of(phiSegments, [&phiExt](ActsScalar phi) {
          return std::abs(phi - phiExt) <
                 std::numeric_limits<ActsScalar>::epsilon();
        })) {
      phiSegments.push_back(phiExt);
    }
  }
  // Add the reference phis
  for (const auto& phiRef : phiRefs) {
    if (phiRef > phiMin && phiRef < phiMax) {
      if (std::ranges::none_of(phiSegments, [&phiRef](ActsScalar phi) {
            return std::abs(phi - phiRef) <
                   std::numeric_limits<ActsScalar>::epsilon();
          })) {
        phiSegments.push_back(phiRef);
      }
    }
  }

  // Sort the phis
  std::ranges::sort(phiSegments);
  return phiSegments;
}

std::vector<Acts::Vector2> Acts::detail::VerticesHelper::ellipsoidVertices(
    ActsScalar innerRx, ActsScalar innerRy, ActsScalar outerRx,
    ActsScalar outerRy, ActsScalar avgPhi, ActsScalar halfPhi,
    unsigned int quarterSegments) {
  // List of vertices counter-clockwise starting at smallest phi w.r.t center,
  // for both inner/outer ring/segment
  std::vector<Vector2> rvertices;  // return vertices
  std::vector<Vector2> ivertices;  // inner vertices
  std::vector<Vector2> overtices;  // outer verices

  bool innerExists = (innerRx > 0. && innerRy > 0.);
  bool closed = std::abs(halfPhi - M_PI) < s_onSurfaceTolerance;

  std::vector<ActsScalar> refPhi = {};
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

std::vector<Acts::Vector2> Acts::detail::VerticesHelper::circularVertices(
    ActsScalar innerR, ActsScalar outerR, ActsScalar avgPhi, ActsScalar halfPhi,
    unsigned int quarterSegments) {
  return ellipsoidVertices(innerR, innerR, outerR, outerR, avgPhi, halfPhi,
                           quarterSegments);
}

bool Acts::detail::VerticesHelper::onHyperPlane(
    const std::vector<Acts::Vector3>& vertices, ActsScalar tolerance) {
  // Obvious always on one surface
  if (vertices.size() < 4) {
    return true;
  }
  // Create the hyperplane
  auto hyperPlane = Eigen::Hyperplane<ActsScalar, 3>::Through(
      vertices[0], vertices[1], vertices[2]);
  for (std::size_t ip = 3; ip < vertices.size(); ++ip) {
    if (hyperPlane.absDistance(vertices[ip]) > tolerance) {
      return false;
    }
  }
  return true;
}
