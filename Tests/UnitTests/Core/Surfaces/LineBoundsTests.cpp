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
  /// test LineBounds(double, double)
  double radius(0.5), halfz(10.);
  LineBounds lineBounds(radius, halfz);
  BOOST_CHECK_EQUAL(lineBounds.type(), SurfaceBounds::eLine);
  /// test copy construction;
  LineBounds copyConstructedLineBounds(lineBounds);  // implicit
  BOOST_CHECK_EQUAL(copyConstructedLineBounds.type(), SurfaceBounds::eLine);
}

/// Unit test for testing LineBounds recreation from streaming
BOOST_AUTO_TEST_CASE(LineBoundsRecreation) {
  double nominalRadius{0.5};
  double nominalHalfLength{20.};
  LineBounds original(nominalRadius, nominalHalfLength);
  LineBounds recreated(original);
  auto valvector = original.values();
  std::array<double, LineBounds::eSize> values{};
  std::copy_n(valvector.begin(), LineBounds::eSize, values.begin());
  BOOST_CHECK_EQUAL(original, recreated);
}

/// Unit test for testing LineBounds exceptions
BOOST_AUTO_TEST_CASE(LineBoundsExceptions) {
  double nominalRadius{0.5};
  double nominalHalfLength{20.};
  // Negative radius
  BOOST_CHECK_THROW(LineBounds(-nominalRadius, nominalHalfLength),
                    std::logic_error);
  // Negative half length
  BOOST_CHECK_THROW(LineBounds(nominalRadius, -nominalHalfLength),
                    std::logic_error);

  // Negative radius and half length
  BOOST_CHECK_THROW(LineBounds(-nominalRadius, -nominalHalfLength),
                    std::logic_error);
}

/// Unit test for testing LineBounds assignment
BOOST_AUTO_TEST_CASE(LineBoundsAssignment) {
  double nominalRadius{0.5};
  double nominalHalfLength{20.};
  LineBounds lineBoundsObject(nominalRadius, nominalHalfLength);
  LineBounds assignedLineBounds = lineBoundsObject;
  BOOST_CHECK_EQUAL(assignedLineBounds, lineBoundsObject);
}

/// Unit tests for LineBounds properties
BOOST_AUTO_TEST_CASE(LineBoundsProperties) {
  // LineBounds object of radius 0.5 and halfz 20
  double nominalRadius{0.5};
  double nominalHalfLength{20.};
  LineBounds lineBoundsObject(nominalRadius, nominalHalfLength);

  /// test for type()
  BOOST_CHECK_EQUAL(lineBoundsObject.type(), SurfaceBounds::eLine);

  /// test for inside()
  const Vector2 origin{0., 0.};
  const Vector2 atRadius{0.5, 10.};
  const Vector2 beyondEnd{0.0, 30.0};
  const Vector2 unitZ{0.0, 1.0};
  const Vector2 unitR{1.0, 0.0};
  const BoundaryTolerance tolerance =
      BoundaryTolerance::AbsoluteBound(0.1, 0.1);
  // This fails because the bounds are not inclusive.
  BOOST_CHECK(!lineBoundsObject.inside(atRadius, tolerance));
  BOOST_CHECK(!lineBoundsObject.inside(beyondEnd, tolerance));
  BOOST_CHECK(lineBoundsObject.inside(unitZ, tolerance));
  BOOST_CHECK(!lineBoundsObject.inside(unitR, tolerance));

  /// Test negative redius inside

  BOOST_CHECK(lineBoundsObject.inside(Vector2{-0.2, 10}, tolerance));
  BOOST_CHECK(!lineBoundsObject.inside(Vector2{-0.8, 10}, tolerance));

  /// test for r()
  BOOST_CHECK_EQUAL(lineBoundsObject.get(LineBounds::eR), nominalRadius);

  /// test for halflengthZ (NOTE: Naming violation)
  BOOST_CHECK_EQUAL(lineBoundsObject.get(LineBounds::eHalfLengthZ),
                    nominalHalfLength);

  /// test for dump
  boost::test_tools::output_test_stream dumpOuput;
  lineBoundsObject.toStream(dumpOuput);
  BOOST_CHECK(dumpOuput.is_equal(
      "Acts::LineBounds: (radius, halflengthInZ) = (0.5000000, 20.0000000)"));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
