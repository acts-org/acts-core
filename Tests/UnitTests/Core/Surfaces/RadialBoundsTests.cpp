// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <algorithm>
#include <array>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

const double rMin = 1.;
const double rMax = 5.;
const double halfPhiSector = std::numbers::pi / 8.;
const double avgPhi = 0.1;

/// Unit tests for RadialBounds constructors
BOOST_AUTO_TEST_CASE(RadialBoundsConstruction) {
  /// Test default construction
  // default construction is deleted

  /// Test construction with radii and default sector
  BOOST_CHECK_EQUAL(RadialBounds(rMin, rMax).type(), SurfaceBounds::eDisc);

  /// Test construction with radii and sector half angle
  BOOST_CHECK_EQUAL(RadialBounds(rMin, rMax, halfPhiSector).type(),
                    SurfaceBounds::eDisc);

  /// Copy constructor
  RadialBounds original(rMin, rMax);
  RadialBounds copied(original);
  BOOST_CHECK_EQUAL(copied, original);
}

// Streaning and recreation test
BOOST_AUTO_TEST_CASE(RadialBoundsRecreation) {
  RadialBounds original(rMin, rMax, halfPhiSector, avgPhi);
  // const bool symmetric(false);
  auto valvector = original.values();
  std::array<double, RadialBounds::eSize> values{};
  std::copy_n(valvector.begin(), RadialBounds::eSize, values.begin());
  RadialBounds recreated(values);
  BOOST_CHECK_EQUAL(original, recreated);
}

// Streaning and recreation test
BOOST_AUTO_TEST_CASE(RadialBoundsException) {
  // Negative inner radius
  BOOST_CHECK_THROW(RadialBounds(-rMin, rMax, halfPhiSector, avgPhi),
                    std::logic_error);

  // Negative outer radius
  BOOST_CHECK_THROW(RadialBounds(rMin, -rMax, halfPhiSector, avgPhi),
                    std::logic_error);

  // Negative inner and outer radius
  BOOST_CHECK_THROW(RadialBounds(-rMin, -rMax, halfPhiSector, avgPhi),
                    std::logic_error);

  // Swapped radii
  BOOST_CHECK_THROW(RadialBounds(rMax, rMin, halfPhiSector, avgPhi),
                    std::logic_error);

  // Out of bound phi sector
  BOOST_CHECK_THROW(RadialBounds(rMin, -rMax, -5., avgPhi), std::logic_error);

  // Out of bound phi position
  BOOST_CHECK_THROW(RadialBounds(rMin, -rMax, halfPhiSector, 5.),
                    std::logic_error);
}

/// Unit tests for RadialBounds properties
BOOST_AUTO_TEST_CASE(RadialBoundsProperties) {
  /// Test type() (redundant; already used in constructor confirmation)
  RadialBounds radialBoundsObject(rMin, rMax, halfPhiSector);
  BOOST_CHECK_EQUAL(radialBoundsObject.type(), SurfaceBounds::eDisc);

  /// Test distanceToBoundary
  Vector2 outside(30., 0.);
  Vector2 inSurface(2., 0.);

  /// Test dump
  boost::test_tools::output_test_stream dumpOutput;
  radialBoundsObject.toStream(dumpOutput);
  BOOST_CHECK(
      dumpOutput.is_equal("Acts::RadialBounds:  (innerRadius, outerRadius, "
                          "hPhiSector, averagePhi) = (1.0000000, "
                          "5.0000000, 0.3926991, 0.0000000)"));

  /// Test inside
  BOOST_CHECK(radialBoundsObject.inside(inSurface, BoundaryTolerance::None()));
  BOOST_CHECK(!radialBoundsObject.inside(outside, BoundaryTolerance::None()));

  /// Test rMin
  BOOST_CHECK_EQUAL(radialBoundsObject.get(RadialBounds::eMinR), rMin);

  /// Test rMax
  BOOST_CHECK_EQUAL(radialBoundsObject.get(RadialBounds::eMaxR), rMax);

  /// Test averagePhi (should be a redundant method, this is not configurable)
  BOOST_CHECK_EQUAL(radialBoundsObject.get(RadialBounds::eAveragePhi), 0.);

  /// Test halfPhiSector
  BOOST_CHECK_EQUAL(radialBoundsObject.get(RadialBounds::eHalfPhiSector),
                    halfPhiSector);
}
/// Unit test for testing RadialBounds assignment
BOOST_AUTO_TEST_CASE(RadialBoundsAssignment) {
  RadialBounds radialBoundsObject(rMin, rMax, halfPhiSector);

  /// Test operator ==
  // not implemented in this class

  /// Test assignment
  RadialBounds assignedRadialBoundsObject(10.1, 123.);
  assignedRadialBoundsObject = radialBoundsObject;
  BOOST_CHECK_EQUAL(assignedRadialBoundsObject, radialBoundsObject);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
