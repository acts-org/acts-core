// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/DiscTrapezoidBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

const double minHalfX = 1.;
const double maxHalfX = 5.;
const double rMin = 2.;
const double rMax = 6.;
const double averagePhi = 0.;
const double stereo = 0.1;

/// Unit tests for DiscTrapezoidBounds constructors
BOOST_AUTO_TEST_CASE(DiscTrapezoidBoundsConstruction) {
  /// Test construction with dimensions and default stereo
  BOOST_CHECK_EQUAL(
      DiscTrapezoidBounds(minHalfX, maxHalfX, rMin, rMax, averagePhi).type(),
      SurfaceBounds::eDiscTrapezoid);

  /// Test construction with all dimensions
  BOOST_CHECK_EQUAL(
      DiscTrapezoidBounds(minHalfX, maxHalfX, rMin, rMax, averagePhi, stereo)
          .type(),
      SurfaceBounds::eDiscTrapezoid);

  /// Copy constructor
  DiscTrapezoidBounds original(minHalfX, maxHalfX, rMin, rMax, averagePhi);
  DiscTrapezoidBounds copied(original);
  BOOST_CHECK_EQUAL(copied.type(), SurfaceBounds::eDiscTrapezoid);
}

// Streaning and recreation test
BOOST_AUTO_TEST_CASE(DiscTrapezoidBoundsRecreation) {
  DiscTrapezoidBounds original(minHalfX, maxHalfX, rMin, rMax, averagePhi,
                               stereo);
  auto valvector = original.values();
  std::array<double, DiscTrapezoidBounds::eSize> values{};
  std::copy_n(valvector.begin(), DiscTrapezoidBounds::eSize, values.begin());
  DiscTrapezoidBounds recreated(values);
  BOOST_CHECK_EQUAL(recreated, original);
}

// Unit tests for AnnulusBounds exception throwing
BOOST_AUTO_TEST_CASE(DiscTrapezoidBoundsExceptions) {
  // Exception for opening neg min half x < 0
  BOOST_CHECK_THROW(
      DiscTrapezoidBounds(-minHalfX, maxHalfX, rMin, rMax, averagePhi, stereo),
      std::logic_error);

  // Exception for opening neg max half x < 0
  BOOST_CHECK_THROW(
      DiscTrapezoidBounds(minHalfX, -maxHalfX, rMin, rMax, averagePhi, stereo),
      std::logic_error);

  // Exception for opening neg min and max half x < 0
  BOOST_CHECK_THROW(
      DiscTrapezoidBounds(-minHalfX, -maxHalfX, rMin, rMax, averagePhi, stereo),
      std::logic_error);

  // Exception for opening neg r min
  BOOST_CHECK_THROW(
      DiscTrapezoidBounds(minHalfX, maxHalfX, -rMin, rMax, averagePhi, stereo),
      std::logic_error);

  // Exception for opening neg r max
  BOOST_CHECK_THROW(
      DiscTrapezoidBounds(minHalfX, maxHalfX, rMin, -rMax, averagePhi, stereo),
      std::logic_error);

  // Exception for opening neg r min and r max
  BOOST_CHECK_THROW(
      DiscTrapezoidBounds(minHalfX, maxHalfX, -rMin, -rMax, averagePhi, stereo),
      std::logic_error);

  // Exception for out of bound average phi
  BOOST_CHECK_THROW(
      DiscTrapezoidBounds(minHalfX, maxHalfX, rMin, rMax, 4., stereo),
      std::logic_error);
}

/// Unit tests for DiscTrapezoidBounds properties
BOOST_AUTO_TEST_CASE(DiscTrapezoidBoundsProperties) {
  /// Test clone
  DiscTrapezoidBounds DiscTrapezoidBoundsObject(minHalfX, maxHalfX, rMin, rMax,
                                                averagePhi);

  /// Test type() (redundant; already used in constructor confirmation)
  BOOST_CHECK_EQUAL(DiscTrapezoidBoundsObject.type(),
                    SurfaceBounds::eDiscTrapezoid);

  /// Test distanceToBoundary
  Vector2 origin(0., 0.);
  Vector2 outside(30., 0.);
  Vector2 inSurface(2.5, 0.);

  /// Test dump
  boost::test_tools::output_test_stream dumpOutput;
  DiscTrapezoidBoundsObject.toStream(dumpOutput);
  BOOST_CHECK(dumpOutput.is_equal(
      "Acts::DiscTrapezoidBounds: (innerRadius, outerRadius, halfLengthXminR, "
      "halfLengthXmaxR, halfLengthY, halfPhiSector, averagePhi, rCenter, "
      "stereo) = "
      "(2.0000000, 6.0000000, 1.0000000, 5.0000000, 0.7922870, 0.9851108, "
      "0.0000000, 2.5243378, 0.0000000)"));

  /// Test inside
  BOOST_CHECK(
      DiscTrapezoidBoundsObject.inside(inSurface, BoundaryTolerance::None()));
  BOOST_CHECK(
      !DiscTrapezoidBoundsObject.inside(outside, BoundaryTolerance::None()));

  /// Test rMin
  CHECK_CLOSE_REL(DiscTrapezoidBoundsObject.rMin(), rMin, 1e-6);

  /// Test rMax
  CHECK_CLOSE_REL(DiscTrapezoidBoundsObject.rMax(), rMax, 1e-6);

  /// Test averagePhi
  CHECK_SMALL(DiscTrapezoidBoundsObject.get(DiscTrapezoidBounds::eAveragePhi),
              1e-9);

  /// Test rCenter (redundant; not configurable)
  CHECK_CLOSE_REL(DiscTrapezoidBoundsObject.rCenter(), 2.524337798, 1e-6);

  /// Test halfPhiSector (redundant; not configurable)
  CHECK_SMALL(DiscTrapezoidBoundsObject.stereo(), 1e-6);

  /// Test minHalfLengthX
  CHECK_CLOSE_REL(
      DiscTrapezoidBoundsObject.get(DiscTrapezoidBounds::eHalfLengthXminR),
      minHalfX, 1e-6);

  /// Test maxHalfLengthX
  CHECK_CLOSE_REL(
      DiscTrapezoidBoundsObject.get(DiscTrapezoidBounds::eHalfLengthXmaxR),
      maxHalfX, 1e-6);

  /// Test halfLengthY
  CHECK_CLOSE_REL(DiscTrapezoidBoundsObject.halfLengthY(), 0.792286991, 1e-6);
}
/// Unit test for testing DiscTrapezoidBounds assignment
BOOST_AUTO_TEST_CASE(DiscTrapezoidBoundsAssignment) {
  DiscTrapezoidBounds DiscTrapezoidBoundsObject(minHalfX, maxHalfX, rMin, rMax,
                                                averagePhi, stereo);

  /// Test operator ==
  // not implemented in this class

  /// Test assignment
  DiscTrapezoidBounds assignedDiscTrapezoidBoundsObject(2.1, 6.6, 3.4, 4.2, 0.3,
                                                        0.);
  assignedDiscTrapezoidBoundsObject = DiscTrapezoidBoundsObject;
  BOOST_CHECK_EQUAL(assignedDiscTrapezoidBoundsObject,
                    DiscTrapezoidBoundsObject);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
