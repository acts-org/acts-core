// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Tests/CommonHelpers/CubicTrackingGeometry.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();

BOOST_AUTO_TEST_CASE(CylindricalTrackingGeometryTest) {
  CylindricalTrackingGeometry cGeometry(tgContext);
  auto tGeometry = cGeometry();
  BOOST_CHECK_NE(tGeometry, nullptr);
}

BOOST_AUTO_TEST_CASE(CubicTrackingGeometryTest) {
  CubicTrackingGeometry cGeometry(tgContext);
  auto tGeometry = cGeometry();
  BOOST_CHECK_NE(tGeometry, nullptr);
}

}  // namespace Acts::Test
