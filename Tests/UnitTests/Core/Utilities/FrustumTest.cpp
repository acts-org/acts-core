// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Frustum.hpp"
#include "Acts/Visualization/PlyVisualization3D.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <utility>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Utilities)
BOOST_AUTO_TEST_CASE(frustum_construction) {
  boost::test_tools::output_test_stream output;

  using Vector2F = Eigen::Matrix<float, 2, 1>;

  using Frustum2f2 = Frustum<float, 2, 2>;
  Frustum2f2 fr({1, 0}, {0, 2}, std::numbers::pi / 2.);

  BOOST_CHECK_EQUAL(fr.origin(), Vector2F(1, 0));
  CHECK_CLOSE_ABS(fr.dir(), Vector2F(0, 1), 1e-6);

  const auto& normals = fr.normals();
  BOOST_CHECK_EQUAL(normals.size(), 3u);

  fr.svg(output, 200, 200);
  BOOST_CHECK(!output.is_empty(true));

  using Vector3F = Eigen::Matrix<float, 3, 1>;

  using Frustum3f3 = Frustum<float, 3, 3>;
  Frustum3f3 fr33({1, 0, 0}, {0, 2, 1}, std::numbers::pi / 2.);

  BOOST_CHECK_EQUAL(fr33.origin(), Vector3F(1, 0, 0));
  CHECK_CLOSE_ABS(fr33.dir(), Vector3F(0, 2, 1).normalized(), 1e-6);

  const auto& normals33 = fr33.normals();
  BOOST_CHECK_EQUAL(normals33.size(), 4u);

  PlyVisualization3D<float> hlp;
  // compile call to draw, does not actually test anything
  // fr33.draw(hlp);

  using Frustum3f4 = Frustum<float, 3, 4>;
  Frustum3f4 fr34({1, 0, 0}, {0, 2, 1}, std::numbers::pi / 2.);

  BOOST_CHECK_EQUAL(fr34.origin(), Vector3F(1, 0, 0));
  CHECK_CLOSE_ABS(fr34.dir(), Vector3F(0, 2, 1).normalized(), 1e-6);

  const auto& normals34 = fr34.normals();
  BOOST_CHECK_EQUAL(normals34.size(), 5u);

  // fr34.draw(hlp);
}
BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
