// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <algorithm>
#include <array>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)
/// Unit test for creating compliant/non-compliant CylinderBounds object

BOOST_AUTO_TEST_CASE(CylinderBoundsConstruction) {
  /// Test default construction
  // default construction is deleted

  const double radius = 0.5;
  const double halfZ = 10.;
  const double halfPhi = std::numbers::pi / 2.;
  const double averagePhi = std::numbers::pi / 2.;
  const double bevelMinZ = -std::numbers::pi / 4.;
  const double bevelMaxZ = std::numbers::pi / 6.;

  BOOST_CHECK_EQUAL(CylinderBounds(radius, halfZ).type(),
                    SurfaceBounds::eCylinder);
  BOOST_CHECK_EQUAL(CylinderBounds(radius, halfZ, halfPhi).type(),
                    SurfaceBounds::eCylinder);
  BOOST_CHECK_EQUAL(CylinderBounds(radius, halfZ, halfPhi, averagePhi).type(),
                    SurfaceBounds::eCylinder);
  BOOST_CHECK_EQUAL(
      CylinderBounds(radius, halfZ, std::numbers::pi, 0., bevelMinZ).type(),
      SurfaceBounds::eCylinder);
  BOOST_CHECK_EQUAL(
      CylinderBounds(radius, halfZ, std::numbers::pi, 0., bevelMinZ, bevelMaxZ)
          .type(),
      SurfaceBounds::eCylinder);

  /// Test copy construction;
  CylinderBounds cylinderBounds(radius, halfZ);
  CylinderBounds copyConstructedCylinderBounds(cylinderBounds);
  BOOST_CHECK_EQUAL(copyConstructedCylinderBounds, cylinderBounds);
}

BOOST_AUTO_TEST_CASE(CylinderBoundsRecreation) {
  const double radius = 0.5;
  const double halfZ = 10.;

  // Test construction with radii and default sector
  auto original = CylinderBounds(radius, halfZ);
  auto valvector = original.values();
  std::array<double, CylinderBounds::eSize> values{};
  std::copy_n(valvector.begin(), CylinderBounds::eSize, values.begin());
  CylinderBounds recreated(values);
  BOOST_CHECK_EQUAL(original, recreated);
}

BOOST_AUTO_TEST_CASE(CylinderBoundsException) {
  const double radius = 0.5;
  const double halfZ = 10.;
  const double halfPhi = std::numbers::pi / 2.;
  const double averagePhi = std::numbers::pi / 2.;

  /// Negative radius
  BOOST_CHECK_THROW(CylinderBounds(-radius, halfZ, halfPhi, averagePhi),
                    std::logic_error);

  /// Negative half length in z
  BOOST_CHECK_THROW(CylinderBounds(radius, -halfZ, halfPhi, averagePhi),
                    std::logic_error);

  /// Negative half sector in phi
  BOOST_CHECK_THROW(CylinderBounds(radius, halfZ, -halfPhi, averagePhi),
                    std::logic_error);

  /// Half sector in phi out of bounds
  BOOST_CHECK_THROW(CylinderBounds(radius, halfZ, 4., averagePhi),
                    std::logic_error);

  /// Phi position out of bounds
  BOOST_CHECK_THROW(CylinderBounds(radius, halfZ, halfPhi, 4.),
                    std::logic_error);
}

/// Unit tests for CylinderBounds properties
BOOST_AUTO_TEST_CASE(CylinderBoundsProperties) {
  // CylinderBounds object of radius 0.5 and halfZ 20
  const double radius = 0.5;
  const double halfZ = 20.;                        // != 10.
  const double halfPhi = std::numbers::pi / 4.;    // != pi/2
  const double averagePhi = 0.;                    // != pi/2
  const double bevelMinZ = std::numbers::pi / 4.;  // != -pi/4
  const double bevelMaxZ = std::numbers::pi / 6.;

  CylinderBounds cylinderBoundsObject(radius, halfZ);
  CylinderBounds cylinderBoundsSegment(radius, halfZ, halfPhi, averagePhi);
  CylinderBounds cylinderBoundsBeveledObject(radius, halfZ, std::numbers::pi,
                                             0., bevelMinZ, bevelMaxZ);

  /// Test for type()
  BOOST_CHECK_EQUAL(cylinderBoundsObject.type(), SurfaceBounds::eCylinder);

  /// Test for inside(), 2D coords are r or phi ,z? : needs clarification
  const Vector2 origin{0., 0.};
  const Vector2 atPiBy2{std::numbers::pi / 2., 0.};
  const Vector2 atPi{std::numbers::pi, 0.};
  const Vector2 beyondEnd{0, 30.};
  const Vector2 unitZ{0., 1.};
  const Vector2 unitPhi{1., 0.};
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

  /// Test for r()
  CHECK_CLOSE_REL(cylinderBoundsObject.get(CylinderBounds::eR), radius, 1e-6);

  /// Test for averagePhi
  CHECK_CLOSE_OR_SMALL(cylinderBoundsObject.get(CylinderBounds::eAveragePhi),
                       averagePhi, 1e-6, 1e-6);

  /// Test for halfPhiSector
  CHECK_CLOSE_REL(cylinderBoundsSegment.get(CylinderBounds::eHalfPhiSector),
                  halfPhi,
                  1e-6);  // fail

  /// Test for halflengthZ (NOTE: Naming violation)
  CHECK_CLOSE_REL(cylinderBoundsObject.get(CylinderBounds::eHalfLengthZ), halfZ,
                  1e-6);

  /// Test for bevelMinZ/MaxZ
  CHECK_CLOSE_REL(cylinderBoundsBeveledObject.get(CylinderBounds::eBevelMinZ),
                  bevelMinZ, 1e-6);
  CHECK_CLOSE_REL(cylinderBoundsBeveledObject.get(CylinderBounds::eBevelMaxZ),
                  bevelMaxZ, 1e-6);

  /// Test for dump
  boost::test_tools::output_test_stream dumpOutput;
  cylinderBoundsObject.toStream(dumpOutput);
  BOOST_CHECK(dumpOutput.is_equal(
      "Acts::CylinderBounds: (radius, halfLengthZ, halfPhiSector, "
      "averagePhi, bevelMinZ, bevelMaxZ) = (0.5000000, 20.0000000, 3.1415927, "
      "0.0000000, 0.0000000, 0.0000000)"));
}

/// Unit test for testing CylinderBounds assignment
BOOST_AUTO_TEST_CASE(CylinderBoundsAssignment) {
  const double radius = 0.5;
  const double halfZ = 20.;  // != 10.

  CylinderBounds cylinderBoundsObject(radius, halfZ);
  CylinderBounds assignedCylinderBounds(10.5, 6.6);
  assignedCylinderBounds = cylinderBoundsObject;

  BOOST_CHECK_EQUAL(assignedCylinderBounds.get(CylinderBounds::eR),
                    cylinderBoundsObject.get(CylinderBounds::eR));
  BOOST_CHECK_EQUAL(assignedCylinderBounds, cylinderBoundsObject);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
