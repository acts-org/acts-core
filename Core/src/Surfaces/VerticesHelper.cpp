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
    const std::vector<ActsScalar>& phiRefs, ActsScalar phiTolerance) {
  // This is to ensure that the extrema are built regardless of number
  // of segments
  std::vector<ActsScalar> phiSegments;
  std::vector<ActsScalar> quarters = {-M_PI, -0.5 * M_PI, 0., 0.5 * M_PI, M_PI};
  // It does not cover the full azimuth
  if (phiMin != -M_PI || phiMax != M_PI) {
    phiSegments.push_back(phiMin);
    for (unsigned int iq = 1; iq < 4; ++iq) {
      if (phiMin < quarters[iq] && phiMax > quarters[iq]) {
        phiSegments.push_back(quarters[iq]);
      }
    }
    phiSegments.push_back(phiMax);
  } else {
    phiSegments = quarters;
  }
  // Insert the reference phis if
  if (!phiRefs.empty()) {
    for (const auto& phiRef : phiRefs) {
      // Trying to find the right patch
      auto match = std::find_if(
          phiSegments.begin(), phiSegments.end(), [&](ActsScalar phiSeg) {
            return std::abs(phiSeg - phiRef) < phiTolerance;
          });
      if (match == phiSegments.end()) {
        phiSegments.push_back(phiRef);
      }
    }
    std::ranges::sort(phiSegments);
  }
  return phiSegments;
}

std::vector<Acts::Vector2> Acts::detail::VerticesHelper::ellipsoidVertices(
    ActsScalar innerRx, ActsScalar innerRy, ActsScalar outerRx,
    ActsScalar outerRy, ActsScalar avgPhi, ActsScalar halfPhi,
    unsigned int lseg) {
  // List of vertices counter-clockwise starting at smallest phi w.r.t center,
  // for both inner/outer ring/segment
  std::vector<Vector2> rvertices;  // return vertices
  std::vector<Vector2> ivertices;  // inner vertices
  std::vector<Vector2> overtices;  // outer verices

  bool innerExists = (innerRx > 0. && innerRy > 0.);
  bool closed = std::abs(halfPhi - M_PI) < s_onSurfaceTolerance;

  // Get the phi segments from the helper method
  auto phiSegs = detail::VerticesHelper::phiSegments(
      avgPhi - halfPhi, avgPhi + halfPhi, {avgPhi});

  // The inner (if exists) and outer bow
  for (unsigned int iseg = 0; iseg < phiSegs.size() - 1; ++iseg) {
    int addon = (iseg == phiSegs.size() - 2 && !closed) ? 1 : 0;
    if (innerExists) {
      createSegment<Vector2, Transform2>(ivertices, {innerRx, innerRy},
                                         phiSegs[iseg], phiSegs[iseg + 1], lseg,
                                         addon);
    }
    createSegment<Vector2, Transform2>(overtices, {outerRx, outerRy},
                                       phiSegs[iseg], phiSegs[iseg + 1], lseg,
                                       addon);
  }

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
    unsigned int lseg) {
  return ellipsoidVertices(innerR, innerR, outerR, outerR, avgPhi, halfPhi,
                           lseg);
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
