// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)
/// Unit test for creating compliant/non-compliant LineBounds object
BOOST_AUTO_TEST_CASE(LineBoundsConstruction) {
  const double radius = 0.5;
  const double halfZ = 10.;

  /// Test LineBounds(double, double)
  LineBounds lineBounds(radius, halfZ);
  BOOST_CHECK_EQUAL(lineBounds.type(), SurfaceBounds::eLine);

  /// Test copy construction;
  LineBounds copyConstructedLineBounds(lineBounds);  // implicit
  BOOST_CHECK_EQUAL(copyConstructedLineBounds.type(), SurfaceBounds::eLine);
}

/// Unit test for testing LineBounds recreation from streaming
BOOST_AUTO_TEST_CASE(LineBoundsRecreation) {
  const double radius = 0.5;
  const double halfZ = 20.;  // != 10.

  LineBounds original(radius, halfZ);
  LineBounds recreated(original);
  auto valvector = original.values();
  std::array<double, LineBounds::eSize> values{};
  std::copy_n(valvector.begin(), LineBounds::eSize, values.begin());
  BOOST_CHECK_EQUAL(original, recreated);
}

/// Unit test for testing LineBounds exceptions
BOOST_AUTO_TEST_CASE(LineBoundsExceptions) {
  const double radius = 0.5;
  const double halfZ = 20.;  // != 10.

  // Negative radius
  BOOST_CHECK_THROW(LineBounds(-radius, halfZ), std::logic_error);

  // Negative half length
  BOOST_CHECK_THROW(LineBounds(radius, -halfZ), std::logic_error);

  // Negative radius and half length
  BOOST_CHECK_THROW(LineBounds(-radius, -halfZ), std::logic_error);
}

/// Unit test for testing LineBounds assignment
BOOST_AUTO_TEST_CASE(LineBoundsAssignment) {
  const double radius = 0.5;
  const double halfZ = 20.;  // != 10.

  LineBounds lineBoundsObject(radius, halfZ);
  LineBounds assignedLineBounds = lineBoundsObject;
  BOOST_CHECK_EQUAL(assignedLineBounds, lineBoundsObject);
}

/// Unit tests for LineBounds properties
BOOST_AUTO_TEST_CASE(LineBoundsProperties) {
  const double radius = 0.5;
  const double halfZ = 20.;  // != 10.

  LineBounds lineBoundsObject(radius, halfZ);

  /// Test for type()
  BOOST_CHECK_EQUAL(lineBoundsObject.type(), SurfaceBounds::eLine);

  /// Test for inside()
  const Vector2 origin{0., 0.};
  const Vector2 atRadius{0.5, 10.};
  const Vector2 beyondEnd{0., 30.};
  const Vector2 unitZ{0., 1.};
  const Vector2 unitR{1., 0.};
  const BoundaryTolerance tolerance =
      BoundaryTolerance::AbsoluteBound(0.1, 0.1);
  // This fails because the bounds are not inclusive.
  BOOST_CHECK(!lineBoundsObject.inside(atRadius, tolerance));
  BOOST_CHECK(!lineBoundsObject.inside(beyondEnd, tolerance));
  BOOST_CHECK(lineBoundsObject.inside(unitZ, tolerance));
  BOOST_CHECK(!lineBoundsObject.inside(unitR, tolerance));

  /// Test negative radius inside
  BOOST_CHECK(lineBoundsObject.inside(Vector2{-0.2, 10}, tolerance));
  BOOST_CHECK(!lineBoundsObject.inside(Vector2{-0.8, 10}, tolerance));

  /// Test for r()
  BOOST_CHECK_EQUAL(lineBoundsObject.get(LineBounds::eR), radius);

  /// Test for halfLengthZ
  BOOST_CHECK_EQUAL(lineBoundsObject.get(LineBounds::eHalfLengthZ), halfZ);

  /// Test for dump
  boost::test_tools::output_test_stream dumpOutput;
  lineBoundsObject.toStream(dumpOutput);
  BOOST_CHECK(dumpOutput.is_equal(
      "Acts::LineBounds: (radius, halflengthInZ) = (0.5000000, 20.0000000)"));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
