// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
