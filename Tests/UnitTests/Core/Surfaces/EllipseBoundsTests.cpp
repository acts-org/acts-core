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
#include "Acts/Surfaces/EllipseBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

/// Unit test for creating compliant/non-compliant EllipseBounds object
BOOST_AUTO_TEST_CASE(EllipseBoundsConstruction) {
  double innerRx(10.), innerRy(15.), outerRx(25.), outerRy(30.),
      phiSector(M_PI / 2.), averagePhi(0.);

  // test default construction
  // EllipseBounds defaultConstructedEllipseBounds;  //deleted
  //
  /// Test construction with dimensions
  BOOST_CHECK_EQUAL(
      EllipseBounds(innerRx, innerRy, outerRx, outerRy, phiSector, averagePhi)
          .type(),
      SurfaceBounds::eEllipse);
  //
  /// Copy constructor
  EllipseBounds original(innerRx, innerRy, outerRx, outerRy, phiSector,
                         averagePhi);
  EllipseBounds copied(original);
  BOOST_CHECK_EQUAL(copied, original);
}

// Streaning and recreation test
BOOST_AUTO_TEST_CASE(EllipseBoundsRecreation) {
  double innerRx(10.), innerRy(15.), outerRx(25.), outerRy(30.),
      phiSector(M_PI / 2.), averagePhi(0.);
  // const bool symmetric(false);
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
  double innerRx(10.), innerRy(15.), outerRx(25.), outerRy(30.),
      phiSector(M_PI / 2.), averagePhi(0.);
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
  double innerRx(10.), outerRx(15.), innerRy(15.), outerRy(20.), averagePhi(0.),
      phiSector(M_PI / 2.);
  /// Test clone
  EllipseBounds ellipseBoundsObject(innerRx, innerRy, outerRx, outerRy,
                                    phiSector, averagePhi);
  //
  /// Test type() (redundant; already used in constructor confirmation)
  BOOST_CHECK_EQUAL(ellipseBoundsObject.type(), SurfaceBounds::eEllipse);
  //
  // clone already tested
  //
  /// Test distanceToBoundary
  Vector2 origin(0., 0.);
  Vector2 outsideBy15(0., 30.);
  Vector2 inRectangle(17., 11.);
  //
  /// Test rMinX
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eInnerRx), innerRx);
  //
  /// Test rMinY
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eOuterRx), outerRx);
  //
  /// Test rMaxX
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eInnerRy), innerRy);
  //
  /// Test rMaxY
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eOuterRy), outerRy);
  //
  /// Test averagePhi
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eAveragePhi),
                    averagePhi);
  //
  /// Test vertices
  // std::vector<Vector2> expectedVertices{{15, 0}, {0, 20}, {-15, 0}, {0,
  // -20}}; const auto& actualVertices = ellipseBoundsObject.vertices(4);
  // BOOST_CHECK_EQUAL_COLLECTIONS(actualVertices.cbegin(),
  // actualVertices.cend(),
  //                              expectedVertices.cbegin(),
  //                              expectedVertices.cend());
  //
  /// Test boundingBox
  BOOST_CHECK_EQUAL(ellipseBoundsObject.boundingBox(),
                    RectangleBounds(15., 20.));
  //
  /// Test halfPhiSector
  BOOST_CHECK_EQUAL(ellipseBoundsObject.get(EllipseBounds::eHalfPhiSector),
                    M_PI / 2.);
  //
  /// Test dump
  boost::test_tools::output_test_stream dumpOuput;
  ellipseBoundsObject.toStream(dumpOuput);
  BOOST_CHECK(dumpOuput.is_equal(
      "Acts::EllipseBounds:  (innerRadius0, outerRadius0, innerRadius1, "
      "outerRadius1, hPhiSector, averagePhi) = (10.0000000, 15.0000000, "
      "15.0000000, "
      "20.0000000, 0.0000000, 1.5707963, 0.0000000)"));
  //
  /// Test inside
  BOOST_CHECK(
      !ellipseBoundsObject.inside(inRectangle, BoundaryTolerance::None()));
  // dont understand why this is so:
  BOOST_CHECK(
      !ellipseBoundsObject.inside(outsideBy15, BoundaryTolerance::None()));
}
/// Unit test for testing EllipseBounds assignment
BOOST_AUTO_TEST_CASE(EllipseBoundsAssignment) {
  double innerRx(10.), outerRx(15.), innerRy(15.), outerRy(20.), averagePhi(0.),
      phiSector(M_PI / 2.);
  EllipseBounds ellipseBoundsObject(innerRx, outerRx, innerRy, outerRy,
                                    averagePhi, phiSector);
  EllipseBounds similarlyConstructeEllipseBoundsObject(
      innerRx, outerRx, innerRy, outerRy, averagePhi, phiSector);
  /// Test operator ==
  BOOST_CHECK_EQUAL(ellipseBoundsObject,
                    similarlyConstructeEllipseBoundsObject);
  //
  /// Test assignment
  EllipseBounds assignedEllipseBoundsObject(11., 12., 17., 18., 1.);
  // object, in some sense
  assignedEllipseBoundsObject = ellipseBoundsObject;
  BOOST_CHECK_EQUAL(assignedEllipseBoundsObject, ellipseBoundsObject);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
