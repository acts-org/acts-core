// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/detail/RealQuadraticEquation.hpp"

using Acts::detail::RealQuadraticEquation;

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)
/// Unit test for creating RealQuadraticEquation object
BOOST_AUTO_TEST_CASE(RealQuadraticEquationConstruction) {
  double a(1.0), b(-3.), c(2.);
  // test default construction: not deleted, not written
  // RealQuadraticEquation defaultConstructedRealQuadraticEquation;
  //
  /// Test construction with parameters
  BOOST_REQUIRE_NO_THROW(RealQuadraticEquation(a, b, c));
  //
  /// Copy constructor (implicit), void removes 'unused' compiler warning
  RealQuadraticEquation orig(a, b, c);
  BOOST_REQUIRE_NO_THROW(RealQuadraticEquation copied(orig); (void)copied);
}
/// Unit test for RealQuadraticEquation properties
BOOST_AUTO_TEST_CASE(RealQuadraticEquationProperties) {
  double a(1.0), b(-3.), c(2.);
  //
  /// Test construction with parameters
  RealQuadraticEquation equation(a, b, c);
  //
  /// Test for solutions
  CHECK_CLOSE_REL(equation.first, 2., 1e-6);
  CHECK_CLOSE_REL(equation.second, 1., 1e-6);
  BOOST_CHECK_EQUAL(equation.solutions, 2);
}

/// Unit test for testing RealQuadraticEquation assignment
BOOST_AUTO_TEST_CASE(RealQuadraticEquationAssignment) {
  double a(1.0), b(-3.), c(2.);
  RealQuadraticEquation realQuadraticEquationObject(a, b, c);
  // operator == not implemented in this class
  //
  /// Test assignment (implicit)
  RealQuadraticEquation assignedRealQuadraticEquationObject(9., -3.5, 6.7);
  assignedRealQuadraticEquationObject = realQuadraticEquationObject;
  CHECK_CLOSE_REL(assignedRealQuadraticEquationObject.first, 2., 1e-6);
  CHECK_CLOSE_REL(assignedRealQuadraticEquationObject.second, 1., 1e-6);
  BOOST_CHECK_EQUAL(assignedRealQuadraticEquationObject.solutions, 2);
  /// equality not written and not implicit
  // BOOST_CHECK_EQUAL(assignedRealQuadraticEquationObject,
  //                   realQuadraticEquationObject);
}
BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
