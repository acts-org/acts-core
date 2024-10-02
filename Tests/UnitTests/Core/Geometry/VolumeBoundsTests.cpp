// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Volumes)

BOOST_AUTO_TEST_CASE(VolumeBoundsTest) {
  // Tests if the planes are correctly oriented
  // s_planeXY
  // s_planeYZ
  // s_planeZX

  Vector3 xaxis(1., 0., 0.);
  Vector3 yaxis(0., 1., 0.);
  Vector3 zaxis(0., 0., 1.);

  auto rotXY = s_planeXY.rotation();
  BOOST_CHECK(rotXY.col(0).isApprox(xaxis));
  BOOST_CHECK(rotXY.col(1).isApprox(yaxis));
  BOOST_CHECK(rotXY.col(2).isApprox(zaxis));

  auto rotYZ = s_planeYZ.rotation();
  BOOST_CHECK(rotYZ.col(0).isApprox(yaxis));
  BOOST_CHECK(rotYZ.col(1).isApprox(zaxis));
  BOOST_CHECK(rotYZ.col(2).isApprox(xaxis));

  auto rotZX = s_planeZX.rotation();
  BOOST_CHECK(rotZX.col(0).isApprox(zaxis));
  BOOST_CHECK(rotZX.col(1).isApprox(xaxis));
  BOOST_CHECK(rotZX.col(2).isApprox(yaxis));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
