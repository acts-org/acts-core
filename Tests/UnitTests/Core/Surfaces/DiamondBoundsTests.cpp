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
#include "Acts/Surfaces/DiamondBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)
/// Unit test for creating compliant/non-compliant DiamondBounds object
BOOST_AUTO_TEST_CASE(DiamondBoundsConstruction) {
  double minHalfX(10.), midHalfX(20.), maxHalfX(15.), halfY1(5.), halfY2(7.);
  // test default construction
  // DiamondBounds defaultConstructedDiamondBounds;  //deleted
  //
  /// Test construction with dimensions
  // DiamondBounds d(minHalfX, midHalfX, maxHalfX, halfY1, halfY2);
  BOOST_CHECK_EQUAL(
      DiamondBounds(minHalfX, midHalfX, maxHalfX, halfY1, halfY2).type(),
      SurfaceBounds::eDiamond);
  //
  /// Copy constructor
  DiamondBounds original(minHalfX, midHalfX, maxHalfX, halfY1, halfY2);
  DiamondBounds copied(original);
  BOOST_CHECK_EQUAL(copied.type(), SurfaceBounds::eDiamond);

  // invalid inputs
  BOOST_CHECK_THROW(
      DiamondBounds db(midHalfX, minHalfX, maxHalfX, halfY1, halfY2),
      std::logic_error);
  BOOST_CHECK_THROW(
      DiamondBounds db(minHalfX, maxHalfX, midHalfX, halfY1, halfY2),
      std::logic_error);
}
/// Unit tests for DiamondBounds properties
BOOST_AUTO_TEST_CASE(DiamondBoundsProperties) {
  double minHalfX(10.), midHalfX(50.), maxHalfX(30.), halfY1(10.), halfY2(20.);
  /// Test clone
  DiamondBounds diamondBoundsObject(minHalfX, midHalfX, maxHalfX, halfY1,
                                    halfY2);
  //
  /// Test type() (redundant; already used in constructor confirmation)
  BOOST_CHECK_EQUAL(diamondBoundsObject.type(), SurfaceBounds::eDiamond);
  // //redundant test
  //
  /// Test the half length at negative y
  BOOST_CHECK_EQUAL(diamondBoundsObject.get(DiamondBounds::eHalfLengthXnegY),
                    minHalfX);
  //
  /// Test the half length at the x axis
  BOOST_CHECK_EQUAL(diamondBoundsObject.get(DiamondBounds::eHalfLengthXzeroY),
                    midHalfX);
  //
  /// Test the half length at positive y
  BOOST_CHECK_EQUAL(diamondBoundsObject.get(DiamondBounds::eHalfLengthXposY),
                    maxHalfX);
  //
  /// Test half length into the negative side
  BOOST_CHECK_EQUAL(diamondBoundsObject.get(DiamondBounds::eHalfLengthYneg),
                    halfY1);
  //
  /// Test half length into the positive side
  BOOST_CHECK_EQUAL(diamondBoundsObject.get(DiamondBounds::eHalfLengthYpos),
                    halfY2);
  //
  /// Test boundingBox
  BOOST_CHECK_EQUAL(diamondBoundsObject.boundingBox(),
                    RectangleBounds(Vector2{-50., -10.}, Vector2{50., 20.}));
  //
  // clone already tested
  //
  /// Test distanceToBoundary
  Vector2 origin(0., 0.);
  Vector2 outsideBy10(0., 30.);
  Vector2 inRectangle(15., 0.);

  /// Test dump
  // Acts::DiamondBounds:  (minHlengthX, medHlengthX, maxHlengthX, hlengthY1,
  // hlengthY2 ) = (30.0000000, 10.0000000, 50.0000000, 10.0000000,
  // 20.0000000)
  diamondBoundsObject.toStream(std::cout);
  boost::test_tools::output_test_stream dumpOuput;
  diamondBoundsObject.toStream(dumpOuput);
  BOOST_CHECK(
      dumpOuput.is_equal("Acts::DiamondBounds: (halfXatYneg, halfXatYzero, "
                         "halfXatYpos, halfYneg, halfYpos) = (10.0000000, "
                         "50.0000000, 30.0000000, 10.0000000, 20.0000000)"));
  //
  /// Test inside
  BOOST_CHECK(diamondBoundsObject.inside(origin, BoundaryTolerance::None()));
  // dont understand why this is so:
  BOOST_CHECK(
      !diamondBoundsObject.inside(outsideBy10, BoundaryTolerance::None()));
  //
  /// Test vertices (does this need to be implemented in this class??
  // auto v=diamondBoundsObject.vertices();
  std::vector<Vector2> referenceVertices{
      {-minHalfX, -halfY1}, {minHalfX, -halfY1}, {midHalfX, 0.},
      {maxHalfX, halfY2},   {-maxHalfX, halfY2}, {-midHalfX, 0.}};
  const auto& actualVertices = diamondBoundsObject.vertices();
  BOOST_CHECK_EQUAL_COLLECTIONS(actualVertices.cbegin(), actualVertices.cend(),
                                referenceVertices.cbegin(),
                                referenceVertices.cend());
}
/// Unit test for testing DiamondBounds assignment
BOOST_AUTO_TEST_CASE(DiamondBoundsAssignment) {
  double minHalfX(10.), midHalfX(20.), maxHalfX(15.), halfY1(5.), halfY2(7.);
  DiamondBounds diamondBoundsObject(minHalfX, midHalfX, maxHalfX, halfY1,
                                    halfY2);
  DiamondBounds similarlyConstructeDiamondBoundsObject(
      minHalfX, midHalfX, maxHalfX, halfY1, halfY2);
  /// Test operator ==
  BOOST_CHECK_EQUAL(diamondBoundsObject,
                    similarlyConstructeDiamondBoundsObject);
  //
  /// Test assignment
  DiamondBounds assignedDiamondBoundsObject(
      2 * minHalfX, 2 * midHalfX, 2 * maxHalfX, 2 * halfY1, 2 * halfY2);
  // object, in some sense
  assignedDiamondBoundsObject = diamondBoundsObject;
  BOOST_CHECK_EQUAL(assignedDiamondBoundsObject, diamondBoundsObject);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
