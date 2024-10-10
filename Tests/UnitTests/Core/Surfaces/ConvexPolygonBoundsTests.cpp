// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/ConvexPolygonBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Acts {
class AssertionFailureException;
}  // namespace Acts

using vec2 = Acts::Vector2;
template <int N>
using poly = Acts::ConvexPolygonBounds<N>;

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

BOOST_AUTO_TEST_CASE(ConvexPolygonBoundsConvexity) {
  std::vector<vec2> vertices;
  vertices = {{0, 0}, {1, 0}, {0.2, 0.2}, {0, 1}};
  { BOOST_CHECK_THROW(poly<4> quad(vertices), std::logic_error); }

  vertices = {{0, 0}, {1, 0}, {0.8, 0.8}, {0, 1}};
  {
    // wrong number of vertices
    BOOST_CHECK_THROW(poly<3> trip{vertices}, AssertionFailureException);
  }
  { poly<4> quad = {vertices}; }

  // this one is self intersecting
  vertices = {{0, 0}, {1, 0}, {0.5, 1}, {0.9, 1.2}};
  { BOOST_CHECK_THROW(poly<4> quad{vertices}, std::logic_error); }

  // this one is not
  vertices = {{0, 0}, {1, 0}, {0.9, 1.2}, {0.5, 1}};
  { poly<4> quad = {vertices}; }

  vertices = {{0, 0}, {1, 0}, {0.8, 0.5}, {1, 1}, {0, 1}};
  { BOOST_CHECK_THROW(poly<5> pent(vertices), std::logic_error); }

  vertices = {{0, 0}, {1, 0}, {1.1, 0.5}, {1, 1}, {0, 1}};
  { poly<5> pent{vertices}; }
}

BOOST_AUTO_TEST_CASE(ConvexPolygonBoundsConstruction) {
  std::vector<vec2> vertices;

  // triangle
  vertices = {{0, 0}, {1, 0}, {0.5, 1}};
  poly<3> triangle(vertices);

  RectangleBounds bb = triangle.boundingBox();
  BOOST_CHECK_EQUAL(bb.min(), Vector2(0, 0));
  BOOST_CHECK_EQUAL(bb.max(), Vector2(1., 1));

  BoundaryTolerance tolerance = BoundaryTolerance::None();

  BOOST_CHECK(triangle.inside({0.2, 0.2}, tolerance));
  BOOST_CHECK(!triangle.inside({0.4, 0.9}, tolerance));
  BOOST_CHECK(!triangle.inside({0.8, 0.8}, tolerance));
  BOOST_CHECK(!triangle.inside({0.3, -0.2}, tolerance));

  // rectangular poly
  vertices = {{0, 0}, {1, 0}, {0.9, 1.2}, {0.5, 1}};
  poly<4> quad(vertices);

  bb = quad.boundingBox();
  BOOST_CHECK_EQUAL(bb.min(), Vector2(0, 0));
  BOOST_CHECK_EQUAL(bb.max(), Vector2(1, 1.2));

  BOOST_CHECK(quad.inside({0.2, 0.2}, tolerance));
  BOOST_CHECK(!quad.inside({0.4, 0.9}, tolerance));
  BOOST_CHECK(quad.inside({0.8, 0.8}, tolerance));
  BOOST_CHECK(!quad.inside({0.3, -0.2}, tolerance));
}

BOOST_AUTO_TEST_CASE(ConvexPolygonBoundsRecreation) {
  // rectangular poly
  std::vector<vec2> vertices = {{0, 0}, {1, 0}, {0.9, 1.2}, {0.5, 1}};
  poly<4> original(vertices);

  auto valvector = original.values();
  std::array<double, poly<4>::eSize> values{};
  std::copy_n(valvector.begin(), poly<4>::eSize, values.begin());
  poly<4> recreated(values);
  BOOST_CHECK_EQUAL(original, recreated);

  // Get the vertices back
  auto rvertices = original.vertices();
  BOOST_CHECK_EQUAL(rvertices.size(), 4u);
}

BOOST_AUTO_TEST_CASE(ConvexPolygonBoundsDynamicTest) {
  using poly = ConvexPolygonBounds<PolygonDynamic>;

  std::vector<vec2> vertices;

  // triangle
  vertices = {{0, 0}, {1, 0}, {0.5, 1}};
  poly triangle(vertices);

  RectangleBounds bb = triangle.boundingBox();
  BOOST_CHECK_EQUAL(bb.min(), Vector2(0, 0));
  BOOST_CHECK_EQUAL(bb.max(), Vector2(1., 1));

  BoundaryTolerance tolerance = BoundaryTolerance::None();

  BOOST_CHECK(triangle.inside({0.2, 0.2}, tolerance));
  BOOST_CHECK(!triangle.inside({0.4, 0.9}, tolerance));
  BOOST_CHECK(!triangle.inside({0.8, 0.8}, tolerance));
  BOOST_CHECK(!triangle.inside({0.3, -0.2}, tolerance));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
