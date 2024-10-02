// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Ray.hpp"

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Utilities)
BOOST_AUTO_TEST_CASE(ray_construction) {
  // 2D

  using Vector2F = Eigen::Matrix<float, 2, 1>;

  boost::test_tools::output_test_stream output;

  Vector2F dir2(0.5, 0.5);
  Ray<float, 2> ray2({1, 1}, dir2);
  dir2.normalize();  // after passing to ray

  BOOST_CHECK_EQUAL(ray2.origin(), Vector2F(1, 1));
  CHECK_CLOSE_ABS(ray2.dir(), dir2, 1e-6);
  Vector2F idir2 = 1. / dir2.array();
  CHECK_CLOSE_ABS(ray2.idir().matrix(), idir2, 1e-6);

  ray2.toStream(output);
  BOOST_CHECK(!output.is_empty(true));

  // 3D
  using Vector3F = Eigen::Matrix<float, 3, 1>;

  Vector3F dir3(1, 2, 1);
  Ray<float, 3> ray3({1, 2, 3}, dir3);
  dir3.normalize();  // after passing to ray

  BOOST_CHECK_EQUAL(ray3.origin(), Vector3F(1, 2, 3));
  CHECK_CLOSE_ABS(ray3.dir(), dir3, 1e-6);
  CHECK_CLOSE_ABS(ray3.idir().matrix(), (1. / dir3.array()).matrix(), 1e-6);

  ray3.toStream(output);
  BOOST_CHECK(!output.is_empty(true));

  // compile draw call, doesn't actually test anything
  // PlyVisualization3D hlp;
  // ray3.draw(hlp);
}
BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
