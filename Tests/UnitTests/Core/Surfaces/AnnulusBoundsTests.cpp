// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

ActsScalar minRadius = 7.2;
ActsScalar maxRadius = 12.0;
ActsScalar minPhi = 0.74195;
ActsScalar maxPhi = 1.33970;

Vector2 offset(-2., 2.);

// Unit tests for AnnulusBounds constructors
BOOST_AUTO_TEST_CASE(AnnulusBoundsConstruction) {
  // Test construction with radii and default sector
  auto original = AnnulusBounds(minRadius, maxRadius, minPhi, maxPhi, offset);
  AnnulusBounds copied(original);
  BOOST_CHECK_EQUAL(original, copied);
}

// Unit tests for AnnulusBounds recreation
BOOST_AUTO_TEST_CASE(AnnulusBoundsRecreation) {
  // Test construction with radii and default sector
  auto original = AnnulusBounds(minRadius, maxRadius, minPhi, maxPhi, offset);
  auto valvector = original.values();
  std::array<double, AnnulusBounds::eSize> values{};
  std::copy_n(valvector.begin(), AnnulusBounds::eSize, values.begin());
  AnnulusBounds recreated(values);
  BOOST_CHECK_EQUAL(original, recreated);
}

// Unit tests for AnnulusBounds exception throwing
BOOST_AUTO_TEST_CASE(AnnulusBoundsExcpetion) {
  // Exception for negative inner radius
  BOOST_CHECK_THROW(AnnulusBounds(-1., maxRadius, minPhi, maxPhi, offset),
                    std::logic_error);
  // Exception for negative outer radius
  BOOST_CHECK_THROW(AnnulusBounds(minRadius, -1., minPhi, maxPhi, offset),
                    std::logic_error);
  // Exception for swapped radii
  BOOST_CHECK_THROW(AnnulusBounds(maxRadius, minRadius, minPhi, maxPhi, offset),
                    std::logic_error);
  // Exception for out of range  min phi
  BOOST_CHECK_THROW(AnnulusBounds(minRadius, maxRadius, -4., maxPhi, offset),
                    std::logic_error);
  // Exception for out of range  max phi
  BOOST_CHECK_THROW(AnnulusBounds(minRadius, maxRadius, minPhi, 4., offset),
                    std::logic_error);
  // Exception for out of range  max phi
  BOOST_CHECK_THROW(AnnulusBounds(minRadius, maxRadius, maxPhi, minPhi, offset),
                    std::logic_error);
}

/// Unit tests for AnnulusBounds properties
BOOST_AUTO_TEST_CASE(AnnulusBoundsProperties) {
  /// Test construction with radii and default sector
  AnnulusBounds aBounds(minRadius, maxRadius, minPhi, maxPhi, offset);

  //
  /// Test type() (redundant; already used in constructor confirmation)
  BOOST_CHECK_EQUAL(aBounds.type(), SurfaceBounds::eAnnulus);

  /// Test positions inside/outside
  // - start from cartesian (from test drawing)
  Vector2 inSurfaceXY(7., 7.);
  Vector2 outsideXY1(5., 5.);
  Vector2 outsideXY2(10., 3.);
  Vector2 outsideXY3(10., 10.);
  Vector2 outsideXY4(4., 10.);
  std::vector<Vector2> testPoints = {inSurfaceXY, outsideXY1, outsideXY2,
                                     outsideXY3, outsideXY4};

  auto toStripFrame = [&](const Vector2& xy) -> Vector2 {
    auto shifted = xy + offset;
    double r = VectorHelpers::perp(shifted);
    double phi = VectorHelpers::phi(shifted);
    return Vector2(r, phi);
  };

  BOOST_CHECK(
      aBounds.inside(toStripFrame(inSurfaceXY), BoundaryTolerance::None()));
  BOOST_CHECK(
      !aBounds.inside(toStripFrame(outsideXY1), BoundaryTolerance::None()));
  BOOST_CHECK(
      !aBounds.inside(toStripFrame(outsideXY2), BoundaryTolerance::None()));
  BOOST_CHECK(
      !aBounds.inside(toStripFrame(outsideXY3), BoundaryTolerance::None()));
  BOOST_CHECK(
      !aBounds.inside(toStripFrame(outsideXY4), BoundaryTolerance::None()));

  // Check radial inside
  BOOST_CHECK(!aBounds.insideRadialBounds(0.5));
  BOOST_CHECK(aBounds.insideRadialBounds(9.));
  BOOST_CHECK(!aBounds.insideRadialBounds(18.));

  // Test rMin
  BOOST_CHECK_EQUAL(aBounds.get(AnnulusBounds::eMinR), minRadius);
  // Test rMax
  BOOST_CHECK_EQUAL(aBounds.get(AnnulusBounds::eMaxR), maxRadius);
  // Test phiMin
  BOOST_CHECK_EQUAL(aBounds.get(AnnulusBounds::eMinPhiRel), minPhi);
  // Test phiMax
  BOOST_CHECK_EQUAL(aBounds.get(AnnulusBounds::eMaxPhiRel), maxPhi);
}

/// Unit tests for AnnulusBounds vertices
BOOST_AUTO_TEST_CASE(AnnulusBoundsVertices) {
  /// Test construction with radii and default sector
  AnnulusBounds aBounds(minRadius, maxRadius, minPhi, maxPhi, offset);

  // Retrieve the corners
  auto corners = aBounds.corners();
  BOOST_CHECK_EQUAL(corners.size(), 4);

  // Retrieve the vertices
  auto vertices = aBounds.vertices(0u);
  BOOST_CHECK_EQUAL(vertices.size(), 4);

  // Now generate with more segments
  unsigned int nQuarterSegments = 12;
  vertices = aBounds.vertices(nQuarterSegments);
  BOOST_CHECK_EQUAL(vertices.size(), 14u);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
