// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/ConeBounds.hpp"
#include "Acts/Surfaces/ConeSurface.hpp"
#include "Acts/Surfaces/ConvexPolygonBounds.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiamondBounds.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/DiscTrapezoidBounds.hpp"
#include "Acts/Surfaces/EllipseBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <tuple>
#include <utility>
#include <vector>

using namespace Acts::UnitLiterals;

Acts::Logging::Level logLevel = Acts::Logging::VERBOSE;

namespace Acts::Test {

// Create a test context
const GeometryContext tgContext = GeometryContext();

const std::vector<std::tuple<std::string, unsigned int>> testModes = {
    {"Triangulate", 18}, {"Extrema", 1}};

const Transform3 transform = Transform3::Identity();
const double epsAbs = 1e-12;

BOOST_AUTO_TEST_SUITE(PolyhedronSurfaces)

/// Unit tests for Cone Surfaces
BOOST_AUTO_TEST_CASE(ConeSurfacePolyhedrons) {
  ACTS_LOCAL_LOGGER(
      Acts::getDefaultLogger("PolyhedronSurfacesTests", logLevel));
  ACTS_INFO("Test: ConeSurfacePolyhedrons");

  const double hzPos = 35_mm;
  const double hzNeg = -20_mm;
  const double alpha = 0.234;

  const double rMax = hzPos * std::tan(alpha);

  for (const auto& [mode, segments] : testModes) {
    ACTS_INFO("\tMode: " << mode);

    /// The full cone on one side
    {
      auto cone = std::make_shared<ConeBounds>(alpha, 0_mm, hzPos);
      auto oneCone = Surface::makeShared<ConeSurface>(transform, cone);
      auto oneConePh = oneCone->polyhedronRepresentation(tgContext, segments);

      const auto extent = oneConePh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), 0_mm, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0_mm, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), hzPos, epsAbs);

      const unsigned int expectedFaces = 4 * segments;
      BOOST_CHECK_EQUAL(oneConePh.faces.size(), expectedFaces);
      // full segments + overlap at (pi/pi) + tip
      BOOST_CHECK_EQUAL(oneConePh.vertices.size(), expectedFaces + 2);
    }

    /// The full cone on one side
    {
      const double hzpMin = 10_mm;
      const double rMin = hzpMin * std::tan(alpha);

      auto conePiece = std::make_shared<ConeBounds>(alpha, hzpMin, hzPos);
      auto oneConePiece =
          Surface::makeShared<ConeSurface>(transform, conePiece);
      auto oneConePiecePh =
          oneConePiece->polyhedronRepresentation(tgContext, segments);

      const auto extent = oneConePiecePh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), rMin, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), hzpMin, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), hzPos, epsAbs);

      const unsigned int expectedFaces = 4 * segments;
      BOOST_CHECK_EQUAL(oneConePiecePh.faces.size(), expectedFaces);
      BOOST_CHECK_EQUAL(oneConePiecePh.vertices.size(),
                        (expectedFaces + 1) * 2);
    }

    /// The full cone on both sides
    {
      auto coneBoth = std::make_shared<ConeBounds>(alpha, hzNeg, hzPos);
      auto twoCones = Surface::makeShared<ConeSurface>(transform, coneBoth);
      auto twoConesPh = twoCones->polyhedronRepresentation(tgContext, segments);

      const auto extent = twoConesPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), 0_mm, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), hzNeg, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), hzPos, epsAbs);

      const unsigned int expectedFaces = 2 * segments * 4;
      const unsigned int expectedVertices = 2 * (4 * segments + 1) + 1;

      BOOST_CHECK_EQUAL(twoConesPh.faces.size(), expectedFaces);
      BOOST_CHECK_EQUAL(twoConesPh.vertices.size(), expectedVertices);
    }

    /// A centered sectoral cone on both sides
    {
      const double phiSector = 0.358;

      auto sectoralBoth =
          std::make_shared<ConeBounds>(alpha, hzNeg, hzPos, phiSector, 0.);
      auto sectoralCones =
          Surface::makeShared<ConeSurface>(transform, sectoralBoth);
      auto sectoralConesPh =
          sectoralCones->polyhedronRepresentation(tgContext, segments);

      const auto extent = sectoralConesPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), 0, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(),
                      -rMax * std::sin(phiSector), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(),
                      rMax * std::sin(phiSector), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), 0_mm, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), rMax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), hzNeg, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), hzPos, epsAbs);

      // Segment numbers are further checked with the VertexHelper checks
    }
  }
}

/// Unit tests for Cylinder Surfaces
BOOST_AUTO_TEST_CASE(CylinderSurfacePolyhedrons) {
  ACTS_LOCAL_LOGGER(
      Acts::getDefaultLogger("PolyhedronSurfacesTests", logLevel));
  ACTS_INFO("Test: CylinderSurfacePolyhedrons");

  const double r = 25_mm;
  const double hZ = 35_mm;

  for (const auto& mode : testModes) {
    ACTS_INFO("\tMode: " << std::get<std::string>(mode));
    const unsigned int segments = std::get<unsigned int>(mode);

    /// The full cone on one side
    {
      auto cylinder = std::make_shared<CylinderBounds>(r, hZ);
      auto fullCylinder =
          Surface::makeShared<CylinderSurface>(transform, cylinder);
      auto fullCylinderPh =
          fullCylinder->polyhedronRepresentation(tgContext, segments);

      const auto extent = fullCylinderPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -r, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), r, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -r, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), r, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), r, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), r, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), -hZ, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), hZ, epsAbs);

      const unsigned int expectedFaces = 4 * segments;
      const unsigned int expectedVertices = (4 * segments + 1) * 2;
      BOOST_CHECK_EQUAL(fullCylinderPh.faces.size(), expectedFaces);
      BOOST_CHECK_EQUAL(fullCylinderPh.vertices.size(), expectedVertices);
    }

    /// The full cone on one side
    {
      const double phiSector = 0.458;

      auto sectorCentered = std::make_shared<CylinderBounds>(r, hZ, phiSector);
      auto centerSectoredCylinder =
          Surface::makeShared<CylinderSurface>(transform, sectorCentered);
      auto centerSectoredCylinderPh =
          centerSectoredCylinder->polyhedronRepresentation(tgContext, segments);

      const auto extent = centerSectoredCylinderPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(),
                      r * std::cos(phiSector), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), r, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(),
                      -r * std::sin(phiSector), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(),
                      r * std::sin(phiSector), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), r, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), r, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), -hZ, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), hZ, epsAbs);
    }
  }
}

/// Unit tests for Disc Surfaces
BOOST_AUTO_TEST_CASE(DiscSurfacePolyhedrons) {
  ACTS_LOCAL_LOGGER(
      Acts::getDefaultLogger("PolyhedronSurfacesTests", logLevel));
  ACTS_INFO("Test: DiscSurfacePolyhedrons");

  const double innerR = 10_mm;
  const double outerR = 25_mm;
  const double phiSector = 0.345;

  for (const auto& mode : testModes) {
    ACTS_INFO("\tMode: " << std::get<std::string>(mode));
    const unsigned int segments = std::get<unsigned int>(mode);

    /// Full disc
    {
      auto disc = std::make_shared<RadialBounds>(0_mm, outerR);
      auto fullDisc = Surface::makeShared<DiscSurface>(transform, disc);
      auto fullDiscPh = fullDisc->polyhedronRepresentation(tgContext, segments);

      const auto extent = fullDiscPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);

      const unsigned int expectedFaces = 1;
      // Segments + overlap + center
      const unsigned int expectedVertices = 4 * segments + 1 + 1;
      BOOST_CHECK_EQUAL(fullDiscPh.faces.size(), expectedFaces);
      BOOST_CHECK_EQUAL(fullDiscPh.vertices.size(), expectedVertices);
    }

    /// Ring disc
    {
      auto radial = std::make_shared<RadialBounds>(innerR, outerR);
      auto radialDisc = Surface::makeShared<DiscSurface>(transform, radial);
      auto radialPh = radialDisc->polyhedronRepresentation(tgContext, segments);

      const auto extent = radialPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), innerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);
    }

    /// Sectoral disc - around 0.
    {
      auto sector = std::make_shared<RadialBounds>(0., outerR, phiSector);
      auto sectorDisc = Surface::makeShared<DiscSurface>(transform, sector);
      auto sectorPh = sectorDisc->polyhedronRepresentation(tgContext, segments);

      const auto extent = sectorPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(),
                      -outerR * std::sin(phiSector), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(),
                      outerR * std::sin(phiSector), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);
    }

    /// Sectoral ring - around 0.
    {
      auto sectorRing =
          std::make_shared<RadialBounds>(innerR, outerR, phiSector);
      auto sectorRingDisc =
          Surface::makeShared<DiscSurface>(transform, sectorRing);
      auto sectorRingDiscPh =
          sectorRingDisc->polyhedronRepresentation(tgContext, segments);

      const auto extent = sectorRingDiscPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(),
                      innerR * std::cos(phiSector), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(),
                      -outerR * std::sin(phiSector), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(),
                      outerR * std::sin(phiSector), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), innerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), outerR, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);
    }

    /// Trapezoid for a disc
    {
      const double halfXmin = 10_mm;
      const double halfXmax = 20_mm;

      auto trapezoidDisc = std::make_shared<DiscTrapezoidBounds>(
          halfXmin, halfXmax, innerR, outerR, 0.);
      auto trapezoidDiscSf =
          Surface::makeShared<DiscSurface>(transform, trapezoidDisc);
      auto trapezoidDiscSfPh =
          trapezoidDiscSf->polyhedronRepresentation(tgContext, segments);
      const auto extent = trapezoidDiscSfPh.extent();

      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(),
                      -std::abs(outerR - innerR) / 2., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(),
                      std::abs(outerR - innerR) / 2., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -halfXmax,
                      epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), halfXmax, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(),
                      std::hypot(std::abs(outerR - innerR) / 2., halfXmax),
                      epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);
    }

    /// AnnulusBounds for a disc
    {
      const double minRadius = 7.;
      const double maxRadius = 12.;
      const double minPhiA = 0.75;
      const double maxPhiA = 1.4;
      const Vector2 offset(0., 0.);

      auto annulus = std::make_shared<AnnulusBounds>(minRadius, maxRadius,
                                                     minPhiA, maxPhiA, offset);
      auto annulusDisc = Surface::makeShared<DiscSurface>(transform, annulus);
      auto annulusDiscPh =
          annulusDisc->polyhedronRepresentation(tgContext, segments);
      const auto extent = annulusDiscPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), minRadius,
                      epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), maxRadius,
                      epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);
    }
  }
}

/// Unit tests for Plane Surfaces
BOOST_AUTO_TEST_CASE(PlaneSurfacePolyhedrons) {
  ACTS_LOCAL_LOGGER(
      Acts::getDefaultLogger("PolyhedronSurfacesTests", logLevel));
  ACTS_INFO("Test: PlaneSurfacePolyhedrons");

  for (const auto& mode : testModes) {
    ACTS_INFO("\tMode: " << std::get<std::string>(mode));
    const unsigned int segments = std::get<unsigned int>(mode);

    /// Rectangular Plane
    {
      const double rhX = 10_mm;
      const double rhY = 25_mm;

      auto rectangular = std::make_shared<RectangleBounds>(rhX, rhY);
      auto rectangularPlane =
          Surface::makeShared<PlaneSurface>(transform, rectangular);
      auto rectangularPh =
          rectangularPlane->polyhedronRepresentation(tgContext, segments);

      const auto extent = rectangularPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -rhX, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), rhX, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -rhY, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), rhY, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(),
                      std::hypot(rhX, rhY), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);

      BOOST_CHECK_EQUAL(rectangularPh.vertices.size(), 4);
      BOOST_CHECK_EQUAL(rectangularPh.faces.size(), 1);

      const std::vector<std::size_t> expectedRect = {0, 1, 2, 3};
      BOOST_CHECK(rectangularPh.faces[0] == expectedRect);
    }

    /// Trapezoidal Plane
    {
      const double thX1 = 10_mm;
      const double thX2 = 25_mm;
      const double thY = 35_mm;

      auto trapezoid = std::make_shared<TrapezoidBounds>(thX1, thX2, thY);
      auto trapezoidalPlane =
          Surface::makeShared<PlaneSurface>(transform, trapezoid);
      auto trapezoidalPh =
          trapezoidalPlane->polyhedronRepresentation(tgContext, segments);

      const auto extent = trapezoidalPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(),
                      -std::max(thX1, thX2), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(),
                      std::max(thX1, thX2), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -thY, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), thY, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(),
                      std::hypot(std::max(thX1, thX2), thY), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);

      BOOST_CHECK_EQUAL(trapezoidalPh.vertices.size(), 4);
      BOOST_CHECK_EQUAL(trapezoidalPh.faces.size(), 1);

      const std::vector<std::size_t> expectedTra = {0, 1, 2, 3};
      BOOST_CHECK(trapezoidalPh.faces[0] == expectedTra);
    }

    /// Ring-like ellipsoidal plane
    {
      const double rMinX = 0_mm;
      const double rMinY = 0_mm;
      const double rMaxX = 30_mm;
      const double rMaxY = 40_mm;
      auto ellipse =
          std::make_shared<EllipseBounds>(rMinX, rMinY, rMaxX, rMaxY);
      auto ellipsoidPlane =
          Surface::makeShared<PlaneSurface>(transform, ellipse);
      auto ellipsoidPh =
          ellipsoidPlane->polyhedronRepresentation(tgContext, segments);

      const auto extent = ellipsoidPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -rMaxX, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), rMaxX, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -rMaxY, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), rMaxY, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(),
                      std::min(rMinX, rMinY), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(),
                      std::max(rMaxX, rMaxY), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);
    }

    {
      const double rMinX = 10_mm;
      const double rMinY = 20_mm;
      const double rMaxX = 30_mm;
      const double rMaxY = 40_mm;
      auto ellipseRing =
          std::make_shared<EllipseBounds>(rMinX, rMinY, rMaxX, rMaxY);
      auto ellipsoidRingPlane =
          Surface::makeShared<PlaneSurface>(transform, ellipseRing);
      auto ellipsoidRingPh =
          ellipsoidRingPlane->polyhedronRepresentation(tgContext, segments);

      const auto extent = ellipsoidRingPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -rMaxX, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), rMaxX, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -rMaxY, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), rMaxY, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(),
                      std::min(rMinX, rMinY), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(),
                      std::max(rMaxX, rMaxY), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);
    }

    /// ConvexPolygonBounds test
    {
      std::vector<Vector2> vtxs = {
          Vector2(-40_mm, -10_mm), Vector2(-10_mm, -30_mm),
          Vector2(30_mm, -20_mm),  Vector2(10_mm, 20_mm),
          Vector2(-20_mm, 50_mm),  Vector2(-30_mm, 30_mm)};

      auto hexagon = std::make_shared<ConvexPolygonBounds<6>>(vtxs);
      auto hexagonPlane = Surface::makeShared<PlaneSurface>(transform, hexagon);
      auto hexagonPlanePh =
          hexagonPlane->polyhedronRepresentation(tgContext, segments);

      const auto extent = hexagonPlanePh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -40, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), 30, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -30, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), 50, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), 0, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(), std::sqrt(2900),
                      epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);
    }

    /// Diamond shaped plane
    {
      const double hMinX = 10_mm;
      const double hMedX = 20_mm;
      const double hMaxX = 15_mm;
      const double hMinY = 40_mm;
      const double hMaxY = 50_mm;

      auto diamond =
          std::make_shared<DiamondBounds>(hMinX, hMedX, hMaxX, hMinY, hMaxY);
      auto diamondPlane = Surface::makeShared<PlaneSurface>(transform, diamond);
      auto diamondPh =
          diamondPlane->polyhedronRepresentation(tgContext, segments);

      const auto extent = diamondPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -hMedX, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), hMedX, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -hMinY, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), hMaxY, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(),
                      std::hypot(hMaxX, hMaxY), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);

      BOOST_CHECK_EQUAL(diamondPh.vertices.size(), 6);
      BOOST_CHECK_EQUAL(diamondPh.faces.size(), 1);
    }
  }
}

/// Unit tests shifted plane
BOOST_AUTO_TEST_CASE(ShiftedSurfacePolyhedrons) {
  ACTS_LOCAL_LOGGER(
      Acts::getDefaultLogger("PolyhedronSurfacesTests", logLevel));
  ACTS_INFO("Test: ShiftedSurfacePolyhedrons");

  const double shiftY = 50_mm;
  Vector3 shift(0., shiftY, 0.);
  Transform3 shiftedTransform = Transform3::Identity();
  shiftedTransform.pretranslate(shift);

  for (const auto& mode : testModes) {
    ACTS_INFO("\tMode: " << std::get<std::string>(mode));
    const unsigned int segments = std::get<unsigned int>(mode);

    /// Rectangular Plane
    {
      const double rhX = 10_mm;
      const double rhY = 25_mm;

      auto rectangular = std::make_shared<RectangleBounds>(rhX, rhY);
      auto rectangularPlane =
          Surface::makeShared<PlaneSurface>(shiftedTransform, rectangular);
      auto rectangularPh =
          rectangularPlane->polyhedronRepresentation(tgContext, segments);

      const auto extent = rectangularPh.extent();
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).min(), -rhX, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binX).max(), rhX, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).min(), -rhY + shiftY,
                      epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binY).max(), rhY + shiftY,
                      epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).min(), 25, epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binR).max(),
                      std::hypot(rhX, rhY + shiftY), epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).min(), 0., epsAbs);
      CHECK_CLOSE_ABS(extent.range(BinningValue::binZ).max(), 0., epsAbs);

      BOOST_CHECK_EQUAL(rectangularPh.vertices.size(), 4);
      BOOST_CHECK_EQUAL(rectangularPh.faces.size(), 1);

      const std::vector<std::size_t> expectedRect = {0, 1, 2, 3};
      BOOST_CHECK(rectangularPh.faces[0] == expectedRect);
    }
  }
}
BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
