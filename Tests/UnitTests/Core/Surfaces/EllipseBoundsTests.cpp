// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/EllipseBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

const double innerRx = 10.;
const double innerRy = 15.;
const double phiSector = std::numbers::pi / 2.;
const double averagePhi = 0.;

/// Unit test for creating compliant/non-compliant EllipseBounds object
BOOST_AUTO_TEST_CASE(EllipseBoundsConstruction) {
  const double outerRx = 25.;
  const double outerRy = 30.;

  /// Test default construction
  // default construction is deleted

  /// Test construction with dimensions
  BOOST_CHECK_EQUAL(
      EllipseBounds(innerRx, innerRy, outerRx, outerRy, phiSector, averagePhi)
          .type(),
      SurfaceBounds::eEllipse);

  /// Copy constructor
  EllipseBounds original(innerRx, innerRy, outerRx, outerRy, phiSector,
                         averagePhi);
  EllipseBounds copied(original);
  BOOST_CHECK_EQUAL(copied, original);
}

// Streaning and recreation test
BOOST_AUTO_TEST_CASE(EllipseBoundsRecreation) {
  const double outerRx = 25.;
  const double outerRy = 30.;

  EllipseBounds original(innerRx, innerRy, outerRx, outerRy, phiSector,
                         averagePhi);
  auto valvector = original.values();
  std::array<double, EllipseBounds::eSize> values{};
  std::copy_n(valvector.begin(), EllipseBounds::eSize, values.begin());
  EllipseBounds recreated(values);
  BOOST_CHECK_EQUAL(recreated, original);
}

// Unit tests for AnnulusBounds exception throwing
BOOST_AUTO_TEST_CASE(ConeBoundsExceptions) {
  const double outerRx = 25.;
  const double outerRy = 30.;

  // Exception for innerRx < 0
  BOOST_CHECK_THROW(
      EllipseBounds(-innerRx, innerRy, outerRx, outerRy, phiSector, averagePhi),
      std::logic_error);

  // Exception for innerRy < 0
  BOOST_CHECK_THROW(
      EllipseBounds(innerRx, -innerRy, outerRx, outerRy, phiSector, averagePhi),
      std::logic_error);

  // Exception for innerRx < 0 and innerRy < 0
  BOOST_CHECK_THROW(EllipseBounds(-innerRx, -innerRy, outerRx, outerRy,
                                  phiSector, averagePhi),
                    std::logic_error);

  // Exception for opening outerRx <= 0
  BOOST_CHECK_THROW(
      EllipseBounds(innerRx, innerRy, 0., outerRy, phiSector, averagePhi),
      std::logic_error);

  // Exception for opening outerRy <= 0
  BOOST_CHECK_THROW(
      EllipseBounds(innerRx, innerRy, outerRx, 0., phiSector, averagePhi),
      std::logic_error);

  // Exception for iouterRx < 0 and outerRy < 0
  BOOST_CHECK_THROW(EllipseBounds(innerRx, innerRy, -outerRx, -outerRy,
                                  phiSector, averagePhi),
                    std::logic_error);

  // Exception for innerRx > outerRx
  BOOST_CHECK_THROW(
      EllipseBounds(outerRx, innerRy, innerRx, outerRy, phiSector, averagePhi),
      std::logic_error);

  // Exception for innerRxy > outerRy
  BOOST_CHECK_THROW(
      EllipseBounds(innerRx, outerRy, outerRx, innerRy, phiSector, averagePhi),
      std::logic_error);

  // Exception for negative phiSector
  BOOST_CHECK_THROW(
      EllipseBounds(innerRx, innerRy, outerRx, outerRy, -phiSector, averagePhi),
      std::logic_error);

  // Exception for average phi out of bound
  BOOST_CHECK_THROW(
      EllipseBounds(innerRx, innerRy, outerRx, outerRy, phiSector, 4.),
      std::logic_error);
}

/// Unit tests for EllipseBounds properties
BOOST_AUTO_TEST_CASE(EllipseBoundsProperties) {
  const double outerRx = 15.;  // != 25
  const double outerRy = 20.;  // != 30

  /// Test clone
  EllipseBounds ellipseBoundsObject(innerRx, innerRy, outerRx, outerRy,
                                    phiSector, averagePhi);

  /// Test type() (redundant; already used in constructor confirmation)
  BOOST_CHECK_EQUAL(ellipseBoundsObject.type(), SurfaceBounds::eEllipse);

  /// Test distanceToBoundary
  Vector2 origin{0., 0.};
  Vector2 outsideBy15{0., 30.};
  Vector2 inRectangle{17., 11.};

  /// Test rMinX
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eInnerRx), innerRx);

  /// Test rMinY
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eOuterRx), outerRx);

  /// Test rMaxX
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eInnerRy), innerRy);

  /// Test rMaxY
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eOuterRy), outerRy);

  /// Test averagePhi
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eAveragePhi),
                    averagePhi);

  /// Test vertices
  // std::vector<Vector2> expectedVertices{{15, 0}, {0, 20}, {-15, 0}, {0,
  // -20}}; const auto& actualVertices = ellipseBoundsObject.vertices(4);
  // BOOST_CHECK_EQUAL_COLLECTIONS(actualVertices.cbegin(),
  // actualVertices.cend(), expectedVertices.cbegin(), expectedVertices.cend());

  /// Test boundingBox
  BOOST_CHECK_EQUAL(ellipseBoundsObject.boundingBox(),
                    RectangleBounds(15., 20.));

  /// Test halfPhiSector
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eHalfPhiSector),
                    std::numbers::pi / 2.);

  /// Test dump
  boost::test_tools::output_test_stream dumpOutput;
  ellipseBoundsObject.toStream(dumpOutput);
  BOOST_CHECK(dumpOutput.is_equal(
      "Acts::EllipseBounds:  (innerRadius0, outerRadius0, innerRadius1, "
      "outerRadius1, hPhiSector, averagePhi) = (10.0000000, 15.0000000, "
      "15.0000000, 20.0000000, 0.0000000, 1.5707963, 0.0000000)"));

  /// Test inside
  BOOST_CHECK(
      !ellipseBoundsObject.inside(inRectangle, BoundaryTolerance::None()));
  // don't understand why this is so:
  BOOST_CHECK(
      !ellipseBoundsObject.inside(outsideBy15, BoundaryTolerance::None()));
}
/// Unit test for testing EllipseBounds assignment
BOOST_AUTO_TEST_CASE(EllipseBoundsAssignment) {
  const double outerRx = 15.;  // != 25
  const double outerRy = 20.;  // != 30

  EllipseBounds ellipseBoundsObject(innerRx, outerRx, innerRy, outerRy,
                                    averagePhi, phiSector);
  EllipseBounds similarlyConstructeEllipseBoundsObject(
      innerRx, outerRx, innerRy, outerRy, averagePhi, phiSector);
  /// Test operator ==
  BOOST_CHECK_EQUAL(ellipseBoundsObject,
                    similarlyConstructeEllipseBoundsObject);

  /// Test assignment
  EllipseBounds assignedEllipseBoundsObject(11., 12., 17., 18., 1.);
  // object, in some sense
  assignedEllipseBoundsObject = ellipseBoundsObject;
  BOOST_CHECK_EQUAL(assignedEllipseBoundsObject, ellipseBoundsObject);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
