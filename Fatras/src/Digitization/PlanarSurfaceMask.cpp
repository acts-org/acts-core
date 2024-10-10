// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsFatras/Digitization/PlanarSurfaceMask.hpp"

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "ActsFatras/Digitization/DigitizationError.hpp"
#include <Acts/Surfaces/AnnulusBounds.hpp>
#include <Acts/Surfaces/DiscTrapezoidBounds.hpp>
#include <Acts/Surfaces/PlanarBounds.hpp>
#include <Acts/Surfaces/RadialBounds.hpp>
#include <Acts/Surfaces/Surface.hpp>
#include <Acts/Utilities/Helpers.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>

namespace {

/// Helper method to check if an intersection is good.
///
/// Good in this context is defined as: along direction,
/// closer than the segment length & reachable
///
/// @param intersections The confirmed intersections for the segment
/// @param candidate The candidate intersection
/// @param sLength The segment length, maximal allowed length
void checkIntersection(std::vector<Acts::Intersection2D>& intersections,
                       const Acts::Intersection2D& candidate, double sLength) {
  if (candidate.isValid() && candidate.pathLength() > 0 &&
      candidate.pathLength() < sLength) {
    intersections.push_back(candidate);
  }
}

/// Helper method to apply the mask and return.
///
/// If two (or more) intersections would be good, apply the first two
/// If only one is available, the boolean tells you which one it is.
/// If no intersection is valid, return an error code for masking.
///
/// @param intersections All confirmed intersections
/// @param segment The original segment before masking
/// @param firstInside Indicator if the first is inside or not
///
/// @return a new Segment (clipped) wrapped in a result or error_code
Acts::Result<ActsFatras::PlanarSurfaceMask::Segment2D> maskAndReturn(
    std::vector<Acts::Intersection2D>& intersections,
    const ActsFatras::PlanarSurfaceMask::Segment2D& segment, bool firstInside) {
  std::ranges::sort(intersections, Acts::Intersection2D::pathLengthOrder);
  if (intersections.size() >= 2) {
    return ActsFatras::PlanarSurfaceMask::Segment2D{
        intersections[0].position(), intersections[1].position()};
  } else if (intersections.size() == 1) {
    return (!firstInside
                ? ActsFatras::PlanarSurfaceMask::Segment2D{intersections[0]
                                                               .position(),
                                                           segment[1]}
                : ActsFatras::PlanarSurfaceMask::Segment2D{
                      segment[0], intersections[0].position()});
  }
  return ActsFatras::DigitizationError::MaskingError;
}

}  // anonymous namespace

Acts::Result<ActsFatras::PlanarSurfaceMask::Segment2D>
ActsFatras::PlanarSurfaceMask::apply(const Acts::Surface& surface,
                                     const Segment2D& segment) const {
  auto surfaceType = surface.type();
  Segment2D clipped(segment);

  // Plane surface section -------------------
  if (surfaceType == Acts::Surface::Plane ||
      surface.bounds().type() == Acts::SurfaceBounds::eDiscTrapezoid) {
    Acts::Vector2 localStart =
        (surfaceType == Acts::Surface::Plane)
            ? segment[0]
            : Acts::Vector2(Acts::VectorHelpers::perp(segment[0]),
                            Acts::VectorHelpers::phi(segment[0]));

    Acts::Vector2 localEnd =
        (surfaceType == Acts::Surface::Plane)
            ? segment[1]
            : Acts::Vector2(Acts::VectorHelpers::perp(segment[1]),
                            Acts::VectorHelpers::phi(segment[1]));

    bool startInside =
        surface.bounds().inside(localStart, Acts::BoundaryTolerance::None());
    bool endInside =
        surface.bounds().inside(localEnd, Acts::BoundaryTolerance::None());

    // Fast exit, both inside
    if (startInside && endInside) {
      return segment;
    }

    // It's either planar or disc trapezoid bounds
    const Acts::PlanarBounds* planarBounds = nullptr;
    const Acts::DiscTrapezoidBounds* dtbBounds = nullptr;
    if (surfaceType == Acts::Surface::Plane) {
      planarBounds =
          static_cast<const Acts::PlanarBounds*>(&(surface.bounds()));
      if (planarBounds->type() == Acts::SurfaceBounds::eEllipse) {
        return DigitizationError::UndefinedSurface;
      }
    } else {
      dtbBounds =
          static_cast<const Acts::DiscTrapezoidBounds*>(&(surface.bounds()));
    }
    auto vertices = planarBounds != nullptr ? planarBounds->vertices(1)
                                            : dtbBounds->vertices(1);

    return polygonMask(vertices, segment, startInside);

  } else if (surfaceType == Acts::Surface::Disc) {
    // Polar coordinates
    Acts::Vector2 sPolar(Acts::VectorHelpers::perp(segment[0]),
                         Acts::VectorHelpers::phi(segment[0]));
    Acts::Vector2 ePolar(Acts::VectorHelpers::perp(segment[1]),
                         Acts::VectorHelpers::phi(segment[1]));

    bool startInside =
        surface.bounds().inside(sPolar, Acts::BoundaryTolerance::None());
    bool endInside =
        surface.bounds().inside(ePolar, Acts::BoundaryTolerance::None());

    // Fast exit for both inside
    if (startInside && endInside) {
      return segment;
    }

    auto boundsType = surface.bounds().type();
    if (boundsType == Acts::SurfaceBounds::eDisc) {
      auto rBounds =
          static_cast<const Acts::RadialBounds*>(&(surface.bounds()));
      return radialMask(*rBounds, segment, {sPolar, ePolar}, startInside);

    } else if (boundsType == Acts::SurfaceBounds::eAnnulus) {
      auto aBounds =
          static_cast<const Acts::AnnulusBounds*>(&(surface.bounds()));
      return annulusMask(*aBounds, segment, startInside);
    }
  }
  return DigitizationError::UndefinedSurface;
}

Acts::Result<ActsFatras::PlanarSurfaceMask::Segment2D>
ActsFatras::PlanarSurfaceMask::polygonMask(
    const std::vector<Acts::Vector2>& vertices, const Segment2D& segment,
    bool firstInside) const {
  std::vector<Acts::Intersection2D> intersections;
  Acts::Vector2 sVector(segment[1] - segment[0]);
  Acts::Vector2 sDir = sVector.normalized();
  double sLength = sVector.norm();

  for (std::size_t iv = 0; iv < vertices.size(); ++iv) {
    const Acts::Vector2& s0 = vertices[iv];
    const Acts::Vector2& s1 =
        (iv + 1) < vertices.size() ? vertices[iv + 1] : vertices[0];
    checkIntersection(
        intersections,
        intersector.intersectSegment(s0, s1, segment[0], sDir, true), sLength);
  }
  return maskAndReturn(intersections, segment, firstInside);
}

Acts::Result<ActsFatras::PlanarSurfaceMask::Segment2D>
ActsFatras::PlanarSurfaceMask::radialMask(const Acts::RadialBounds& rBounds,
                                          const Segment2D& segment,
                                          const Segment2D& polarSegment,
                                          bool firstInside) const {
  double rMin = rBounds.get(Acts::RadialBounds::eMinR);
  double rMax = rBounds.get(Acts::RadialBounds::eMaxR);
  double hPhi = rBounds.get(Acts::RadialBounds::eHalfPhiSector);
  double aPhi = rBounds.get(Acts::RadialBounds::eAveragePhi);

  std::array<double, 2> radii = {rMin, rMax};
  std::array<double, 2> phii = {aPhi - hPhi, aPhi + hPhi};

  std::vector<Acts::Intersection2D> intersections;
  Acts::Vector2 sVector(segment[1] - segment[0]);
  Acts::Vector2 sDir = sVector.normalized();
  double sLength = sVector.norm();

  double sR = polarSegment[0][Acts::eBoundLoc0];
  double eR = polarSegment[1][Acts::eBoundLoc0];
  double sPhi = polarSegment[0][Acts::eBoundLoc1];
  double ePhi = polarSegment[1][Acts::eBoundLoc1];

  // Helper method to intersect phi boundaries
  auto intersectPhiLine = [&](double phi) -> void {
    Acts::Vector2 s0(rMin * std::cos(phi), rMin * std::sin(phi));
    Acts::Vector2 s1(rMax * std::cos(phi), rMax * std::sin(phi));
    checkIntersection(
        intersections,
        intersector.intersectSegment(s0, s1, segment[0], sDir, true), sLength);
  };

  // Helper method to intersect radial full boundaries
  auto intersectCircle = [&](double r) -> void {
    auto cIntersections = intersector.intersectCircle(r, segment[0], sDir);
    for (const auto& intersection : cIntersections) {
      checkIntersection(intersections, intersection, sLength);
    }
  };

  // Intersect phi lines
  if ((M_PI - hPhi) > Acts::s_epsilon) {
    if (sPhi < phii[0] || ePhi < phii[0]) {
      intersectPhiLine(phii[0]);
    }
    if (sPhi > phii[1] || ePhi > phii[1]) {
      intersectPhiLine(phii[1]);
    }
    // Intersect radial segments
    if (sR < radii[0] || eR < radii[0]) {
      checkIntersection(intersections,
                        intersector.intersectCircleSegment(
                            radii[0], phii[0], phii[1], segment[0], sDir),
                        sLength);
    }
    if (sR > radii[1] || eR > radii[1]) {
      checkIntersection(intersections,
                        intersector.intersectCircleSegment(
                            radii[1], phii[0], phii[1], segment[0], sDir),
                        sLength);
    }
  } else {
    // Full radial set
    // Intersect radial segments
    if (sR < radii[0] || eR < radii[0]) {
      intersectCircle(radii[0]);
    }
    if (sR > radii[1] || eR > radii[1]) {
      intersectCircle(radii[1]);
    }
  }
  return maskAndReturn(intersections, segment, firstInside);
}

Acts::Result<ActsFatras::PlanarSurfaceMask::Segment2D>
ActsFatras::PlanarSurfaceMask::annulusMask(const Acts::AnnulusBounds& aBounds,
                                           const Segment2D& segment,
                                           bool firstInside) const {
  auto vertices = aBounds.vertices(0);
  Acts::Vector2 moduleOrigin = aBounds.moduleOrigin();

  std::array<std::array<unsigned int, 2>, 2> edgeCombos = {
      std::array<unsigned int, 2>{0, 3}, std::array<unsigned int, 2>{1, 2}};

  std::vector<Acts::Intersection2D> intersections;
  Acts::Vector2 sVector(segment[1] - segment[0]);
  Acts::Vector2 sDir = sVector.normalized();
  double sLength = sVector.norm();
  // First the phi edges in strip system
  for (const auto& ec : edgeCombos) {
    checkIntersection(
        intersections,
        intersector.intersectSegment(vertices[ec[0]], vertices[ec[1]],
                                     segment[0], sDir, true),
        sLength);
  }

  // Shift them to get the module phi and intersect
  std::array<unsigned int, 4> phii = {1, 0, 2, 3};
  for (unsigned int iarc = 0; iarc < 2; ++iarc) {
    Acts::Intersection2D intersection = intersector.intersectCircleSegment(
        aBounds.get(static_cast<Acts::AnnulusBounds::BoundValues>(iarc)),
        Acts::VectorHelpers::phi(vertices[phii[iarc * 2]] - moduleOrigin),
        Acts::VectorHelpers::phi(vertices[phii[iarc * 2 + 1]] - moduleOrigin),
        segment[0] - moduleOrigin, sDir);
    if (intersection.isValid()) {
      checkIntersection(intersections,
                        Acts::Intersection2D(
                            intersection.position() + moduleOrigin,
                            intersection.pathLength(), intersection.status()),
                        sLength);
    }
  }
  return maskAndReturn(intersections, segment, firstInside);
}
