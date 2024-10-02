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
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <memory>
#include <string>

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();

BOOST_AUTO_TEST_SUITE(PerigeeSurfaces)
/// Unit test for creating compliant/non-compliant PerigeeSurface object
BOOST_AUTO_TEST_CASE(PerigeeSurfaceConstruction) {
  // PerigeeSurface default constructor is deleted
  //
  /// Constructor with Vector3
  Vector3 unitXYZ{1., 1., 1.};
  auto perigeeSurfaceObject = Surface::makeShared<PerigeeSurface>(unitXYZ);
  BOOST_CHECK_EQUAL(Surface::makeShared<PerigeeSurface>(unitXYZ)->type(),
                    Surface::Perigee);
  //
  /// Constructor with transform
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  BOOST_CHECK_EQUAL(Surface::makeShared<PerigeeSurface>(pTransform)->type(),
                    Surface::Perigee);
  //
  /// Copy constructor
  auto copiedPerigeeSurface =
      Surface::makeShared<PerigeeSurface>(*perigeeSurfaceObject);
  BOOST_CHECK_EQUAL(copiedPerigeeSurface->type(), Surface::Perigee);
  BOOST_CHECK(*copiedPerigeeSurface == *perigeeSurfaceObject);
  //
  /// Copied and transformed
  auto copiedTransformedPerigeeSurface = Surface::makeShared<PerigeeSurface>(
      tgContext, *perigeeSurfaceObject, pTransform);
  BOOST_CHECK_EQUAL(copiedTransformedPerigeeSurface->type(), Surface::Perigee);
}
//
/// Unit test for testing PerigeeSurface properties
BOOST_AUTO_TEST_CASE(PerigeeSurfaceProperties) {
  /// Test clone method
  Vector3 unitXYZ{1., 1., 1.};
  auto perigeeSurfaceObject = Surface::makeShared<PerigeeSurface>(unitXYZ);
  //
  /// Test type (redundant)
  BOOST_CHECK_EQUAL(perigeeSurfaceObject->type(), Surface::Perigee);
  //
  /// Test name
  BOOST_CHECK_EQUAL(perigeeSurfaceObject->name(),
                    std::string("Acts::PerigeeSurface"));
  //
  /// Test dump
  boost::test_tools::output_test_stream dumpOutput;
  dumpOutput << perigeeSurfaceObject->toStream(tgContext);
  BOOST_CHECK(
      dumpOutput.is_equal("Acts::PerigeeSurface:\n\
     Center position  (x, y, z) = (1.0000000, 1.0000000, 1.0000000)"));
}

BOOST_AUTO_TEST_CASE(EqualityOperators) {
  Vector3 unitXYZ{1., 1., 1.};
  Vector3 invalidPosition{0.0, 0.0, 0.0};
  auto perigeeSurfaceObject = Surface::makeShared<PerigeeSurface>(unitXYZ);
  auto perigeeSurfaceObject2 = Surface::makeShared<PerigeeSurface>(unitXYZ);
  auto assignedPerigeeSurface =
      Surface::makeShared<PerigeeSurface>(invalidPosition);
  /// Test equality operator
  BOOST_CHECK(*perigeeSurfaceObject == *perigeeSurfaceObject2);
  /// Test assignment
  *assignedPerigeeSurface = *perigeeSurfaceObject;
  /// Test equality of assigned to original
  BOOST_CHECK(*assignedPerigeeSurface == *perigeeSurfaceObject);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Acts::Test
