// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)
/// Unit test for creating compliant/non-compliant CylinderBounds object

BOOST_AUTO_TEST_CASE(CylinderBoundsConstruction) {
  /// test default construction
  // CylinderBounds defaultConstructedCylinderBounds;  // deleted
  double radius(0.5), halfz(10.), halfphi(M_PI / 2.0), averagePhi(M_PI / 2.0),
      minBevelZ(-M_PI / 4), maxBevelZ(M_PI / 6);
  BOOST_CHECK_EQUAL(CylinderBounds(radius, halfz).type(),
                    SurfaceBounds::eCylinder);
  BOOST_CHECK_EQUAL(CylinderBounds(radius, halfz, halfphi).type(),
                    SurfaceBounds::eCylinder);
  BOOST_CHECK_EQUAL(CylinderBounds(radius, halfz, halfphi, averagePhi).type(),
                    SurfaceBounds::eCylinder);
  BOOST_CHECK_EQUAL(CylinderBounds(radius, halfz, M_PI, 0., minBevelZ).type(),
                    SurfaceBounds::eCylinder);
  BOOST_CHECK_EQUAL(
      CylinderBounds(radius, halfz, M_PI, 0., minBevelZ, maxBevelZ).type(),
      SurfaceBounds::eCylinder);
  //
  /// test copy construction;
  CylinderBounds cylinderBounds(radius, halfz);
  CylinderBounds copyConstructedCylinderBounds(cylinderBounds);
  BOOST_CHECK_EQUAL(copyConstructedCylinderBounds, cylinderBounds);
}

BOOST_AUTO_TEST_CASE(CylinderBoundsRecreation) {
  /// test default construction
  // CylinderBounds defaultConstructedCylinderBounds;  // deleted
  double radius(0.5), halfz(10.);
  // Test construction with radii and default sector
  auto original = CylinderBounds(radius, halfz);
  auto valvector = original.values();
  std::array<double, CylinderBounds::eSize> values{};
  std::copy_n(valvector.begin(), CylinderBounds::eSize, values.begin());
  CylinderBounds recreated(values);
  BOOST_CHECK_EQUAL(original, recreated);
}

BOOST_AUTO_TEST_CASE(CylinderBoundsException) {
  double radius(0.5), halfz(10.), halfphi(M_PI / 2.0), averagePhi(M_PI / 2.0);

  // Negative radius
  BOOST_CHECK_THROW(CylinderBounds(-radius, halfz, halfphi, averagePhi),
                    std::logic_error);

  // Negative half length in z
  BOOST_CHECK_THROW(CylinderBounds(radius, -halfz, halfphi, averagePhi),
                    std::logic_error);

  // Negative half sector in phi
  BOOST_CHECK_THROW(CylinderBounds(radius, halfz, -halfphi, averagePhi),
                    std::logic_error);

  // Half sector in phi out of bounds
  BOOST_CHECK_THROW(CylinderBounds(radius, halfz, 4., averagePhi),
                    std::logic_error);

  // Phi position out of bounds
  BOOST_CHECK_THROW(CylinderBounds(radius, halfz, halfphi, 4.),
                    std::logic_error);
}

/// Unit tests for CylinderBounds properties
BOOST_AUTO_TEST_CASE(CylinderBoundsProperties) {
  // CylinderBounds object of radius 0.5 and halfz 20
  double nominalRadius{0.5};
  double nominalHalfLength{20.};
  double halfphi(M_PI / 4.0);
  double averagePhi(0.0);
  double bevelMinZ(M_PI / 4);
  double bevelMaxZ(M_PI / 6);
  CylinderBounds cylinderBoundsObject(nominalRadius, nominalHalfLength);
  CylinderBounds cylinderBoundsSegment(nominalRadius, nominalHalfLength,
                                       halfphi, averagePhi);
  CylinderBounds cylinderBoundsBeveledObject(nominalRadius, nominalHalfLength,
                                             M_PI, 0., bevelMinZ, bevelMaxZ);

  /// test for type()
  BOOST_CHECK_EQUAL(cylinderBoundsObject.type(), SurfaceBounds::eCylinder);

  /// test for inside(), 2D coords are r or phi ,z? : needs clarification
  const Vector2 origin{0., 0.};
  const Vector2 atPiBy2{M_PI / 2., 0.0};
  const Vector2 atPi{M_PI, 0.0};
  const Vector2 beyondEnd{0, 30.0};
  const Vector2 unitZ{0.0, 1.0};
  const Vector2 unitPhi{1.0, 0.0};
  const Vector2 withinBevelMin{0.5, -20.012};
  const Vector2 outsideBevelMin{0.5, -40.};
  const BoundaryTolerance tolerance =
      BoundaryTolerance::AbsoluteBound(0.1, 0.1);
  const BoundaryTolerance lessTolerance =
      BoundaryTolerance::AbsoluteBound(0.01, 0.01);
  BOOST_CHECK(cylinderBoundsObject.inside(atPiBy2, tolerance));
  BOOST_CHECK(!cylinderBoundsSegment.inside(unitPhi, tolerance));
  BOOST_CHECK(cylinderBoundsObject.inside(origin, tolerance));

  BOOST_CHECK(!cylinderBoundsObject.inside(withinBevelMin, lessTolerance));
  BOOST_CHECK(
      cylinderBoundsBeveledObject.inside(withinBevelMin, lessTolerance));
  BOOST_CHECK(
      !cylinderBoundsBeveledObject.inside(outsideBevelMin, lessTolerance));

  /// test for r()
  CHECK_CLOSE_REL(cylinderBoundsObject.get(CylinderBounds::eR), nominalRadius,
                  1e-6);

  /// test for averagePhi
  CHECK_CLOSE_OR_SMALL(cylinderBoundsObject.get(CylinderBounds::eAveragePhi),
                       averagePhi, 1e-6, 1e-6);

  /// test for halfPhiSector
  CHECK_CLOSE_REL(cylinderBoundsSegment.get(CylinderBounds::eHalfPhiSector),
                  halfphi,
                  1e-6);  // fail

  /// test for halflengthZ (NOTE: Naming violation)
  CHECK_CLOSE_REL(cylinderBoundsObject.get(CylinderBounds::eHalfLengthZ),
                  nominalHalfLength, 1e-6);

  /// test for bevelMinZ/MaxZ
  CHECK_CLOSE_REL(cylinderBoundsBeveledObject.get(CylinderBounds::eBevelMinZ),
                  bevelMinZ, 1e-6);
  CHECK_CLOSE_REL(cylinderBoundsBeveledObject.get(CylinderBounds::eBevelMaxZ),
                  bevelMaxZ, 1e-6);

  /// test for dump
  boost::test_tools::output_test_stream dumpOuput;
  cylinderBoundsObject.toStream(dumpOuput);
  BOOST_CHECK(dumpOuput.is_equal(
      "Acts::CylinderBounds: (radius, halfLengthZ, halfPhiSector, "
      "averagePhi, bevelMinZ, bevelMaxZ) = (0.5000000, 20.0000000, 3.1415927, "
      "0.0000000, 0.0000000, 0.0000000)"));
}
/// Unit test for testing CylinderBounds assignment
BOOST_AUTO_TEST_CASE(CylinderBoundsAssignment) {
  double nominalRadius{0.5};
  double nominalHalfLength{20.};
  CylinderBounds cylinderBoundsObject(nominalRadius, nominalHalfLength);
  CylinderBounds assignedCylinderBounds(10.5, 6.6);
  assignedCylinderBounds = cylinderBoundsObject;
  BOOST_CHECK_EQUAL(assignedCylinderBounds.get(CylinderBounds::eR),
                    cylinderBoundsObject.get(CylinderBounds::eR));
  BOOST_CHECK_EQUAL(assignedCylinderBounds, cylinderBoundsObject);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
