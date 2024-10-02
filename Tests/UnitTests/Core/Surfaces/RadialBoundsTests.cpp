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
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

/// Unit tests for RadialBounds constrcuctors
BOOST_AUTO_TEST_CASE(RadialBoundsConstruction) {
  double minRadius(1.0), maxRadius(5.0), halfPhiSector(M_PI / 8.0);
  // test default construction
  // RadialBounds defaultConstructedRadialBounds;  should be deleted
  //
  /// Test construction with radii and default sector
  BOOST_CHECK_EQUAL(RadialBounds(minRadius, maxRadius).type(),
                    SurfaceBounds::eDisc);
  //
  /// Test construction with radii and sector half angle
  BOOST_CHECK_EQUAL(RadialBounds(minRadius, maxRadius, halfPhiSector).type(),
                    SurfaceBounds::eDisc);
  //
  /// Copy constructor
  RadialBounds original(minRadius, maxRadius);
  RadialBounds copied(original);
  BOOST_CHECK_EQUAL(copied, original);
}

// Streaning and recreation test
BOOST_AUTO_TEST_CASE(RadialBoundsRecreation) {
  double minRadius(1.0), maxRadius(5.0), halfPhiSector(M_PI / 8.0), avgPhi(0.1);
  RadialBounds original(minRadius, maxRadius, halfPhiSector, avgPhi);
  // const bool symmetric(false);
  auto valvector = original.values();
  std::array<double, RadialBounds::eSize> values{};
  std::copy_n(valvector.begin(), RadialBounds::eSize, values.begin());
  RadialBounds recreated(values);
  BOOST_CHECK_EQUAL(original, recreated);
}

// Streaning and recreation test
BOOST_AUTO_TEST_CASE(RadialBoundsException) {
  double minRadius(1.0), maxRadius(5.0), halfPhiSector(M_PI / 8.0), avgPhi(0.1);

  // Negative inner radius
  BOOST_CHECK_THROW(RadialBounds(-minRadius, maxRadius, halfPhiSector, avgPhi),
                    std::logic_error);

  // Negative outer radius
  BOOST_CHECK_THROW(RadialBounds(minRadius, -maxRadius, halfPhiSector, avgPhi),
                    std::logic_error);

  // Negative inner and outer radius
  BOOST_CHECK_THROW(RadialBounds(-minRadius, -maxRadius, halfPhiSector, avgPhi),
                    std::logic_error);

  // Swapped radii
  BOOST_CHECK_THROW(RadialBounds(maxRadius, minRadius, halfPhiSector, avgPhi),
                    std::logic_error);

  // Out of bound phi sector
  BOOST_CHECK_THROW(RadialBounds(minRadius, -maxRadius, -5., avgPhi),
                    std::logic_error);

  // Out of bound phi position
  BOOST_CHECK_THROW(RadialBounds(minRadius, -maxRadius, halfPhiSector, 5.),
                    std::logic_error);
}

/// Unit tests for RadialBounds properties
BOOST_AUTO_TEST_CASE(RadialBoundsProperties) {
  double minRadius(1.0), maxRadius(5.0), halfPhiSector(M_PI / 8.0);
  /// Test type() (redundant; already used in constructor confirmation)
  RadialBounds radialBoundsObject(minRadius, maxRadius, halfPhiSector);
  BOOST_CHECK_EQUAL(radialBoundsObject.type(), SurfaceBounds::eDisc);
  //
  /// Test distanceToBoundary
  Vector2 outside(30., 0.);
  Vector2 inSurface(2., 0.0);

  /// Test dump
  boost::test_tools::output_test_stream dumpOuput;
  radialBoundsObject.toStream(dumpOuput);
  BOOST_CHECK(
      dumpOuput.is_equal("Acts::RadialBounds:  (innerRadius, outerRadius, "
                         "hPhiSector, averagePhi) = (1.0000000, "
                         "5.0000000, 0.3926991, 0.0000000)"));
  //
  /// Test inside
  BOOST_CHECK(radialBoundsObject.inside(inSurface, BoundaryTolerance::None()));
  BOOST_CHECK(!radialBoundsObject.inside(outside, BoundaryTolerance::None()));
  //
  /// Test rMin
  BOOST_CHECK_EQUAL(radialBoundsObject.get(RadialBounds::eMinR), minRadius);
  //
  /// Test rMax
  BOOST_CHECK_EQUAL(radialBoundsObject.get(RadialBounds::eMaxR), maxRadius);
  //
  /// Test averagePhi (should be a redundant method, this is not configurable)
  BOOST_CHECK_EQUAL(radialBoundsObject.get(RadialBounds::eAveragePhi), 0.0);
  //
  /// Test halfPhiSector
  BOOST_CHECK_EQUAL(radialBoundsObject.get(RadialBounds::eHalfPhiSector),
                    halfPhiSector);
}
/// Unit test for testing RadialBounds assignment
BOOST_AUTO_TEST_CASE(RadialBoundsAssignment) {
  double minRadius(1.0), maxRadius(5.0), halfPhiSector(M_PI / 8.0);
  RadialBounds radialBoundsObject(minRadius, maxRadius, halfPhiSector);
  // operator == not implemented in this class
  //
  /// Test assignment
  RadialBounds assignedRadialBoundsObject(10.1, 123.);
  assignedRadialBoundsObject = radialBoundsObject;
  BOOST_CHECK_EQUAL(assignedRadialBoundsObject, radialBoundsObject);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
