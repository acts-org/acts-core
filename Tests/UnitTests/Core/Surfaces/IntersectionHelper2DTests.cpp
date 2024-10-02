// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Surfaces/detail/IntersectionHelper2D.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Intersection.hpp"

#include <array>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

void basicChecks(bool circleCase = false) {
  double rY = 10.;
  double rX = circleCase ? rY : 5.;

  // Line along x - not intersecting
  Vector2 start(12., 0.);
  Vector2 direction(0., -1.);

  auto nosol = circleCase ? detail::IntersectionHelper2D::intersectCircle(
                                rY, start, direction)
                          : detail::IntersectionHelper2D::intersectEllipse(
                                rX, rY, start, direction);
  BOOST_CHECK(!nosol[0].isValid());
  BOOST_CHECK(!nosol[1].isValid());

  start = Vector2(4., -4.);
  auto twosol = circleCase ? detail::IntersectionHelper2D::intersectCircle(
                                 rY, start, direction)
                           : detail::IntersectionHelper2D::intersectEllipse(
                                 rX, rY, start, direction);

  BOOST_CHECK(twosol[0].isValid());
  BOOST_CHECK(twosol[1].isValid());

  start = Vector2(-4., 10.);
  direction = Vector2(1., 0.);

  auto onesolY = circleCase ? detail::IntersectionHelper2D::intersectCircle(
                                  rY, start, direction)
                            : detail::IntersectionHelper2D::intersectEllipse(
                                  rX, rY, start, direction);

  BOOST_CHECK(onesolY[0].isValid());
  CHECK_CLOSE_ABS(onesolY[0].position().x(), 0., s_epsilon);
  BOOST_CHECK(!onesolY[1].isValid());

  start = Vector2(rX, -4);
  direction = Vector2(0., 1.);

  auto onesolX = circleCase ? detail::IntersectionHelper2D::intersectCircle(
                                  rY, start, direction)
                            : detail::IntersectionHelper2D::intersectEllipse(
                                  rX, rY, start, direction);

  BOOST_CHECK(onesolX[0].isValid());
  CHECK_CLOSE_ABS(onesolX[0].position().y(), 0., s_epsilon);
  BOOST_CHECK(!onesolX[1].isValid());
}

/// Unit test for creating Ellipse intersection
BOOST_AUTO_TEST_CASE(LineLineIntersection) {
  Vector2 start(1., 1.);
  Vector2 dir(1., 1.);

  // Not possible
  auto solution = detail::IntersectionHelper2D::intersectSegment(
      Vector2(5., 3.), Vector2(6., 4), start, dir.normalized());

  BOOST_CHECK(!solution.isValid());

  // Possible
  solution = detail::IntersectionHelper2D::intersectSegment(
      Vector2(5., 3.), Vector2(3., -1.), start, dir.normalized());

  BOOST_CHECK(solution.isValid());

  // In principle possible, but out of bound
  start = Vector2(2, 3);
  dir = Vector2(2, 1).normalized();

  solution = detail::IntersectionHelper2D::intersectSegment(
      Vector2(-1., -2.5), Vector2(3., 2.5), start, dir);

  BOOST_CHECK(solution.isValid());

  solution = detail::IntersectionHelper2D::intersectSegment(
      Vector2(-1., -2.5), Vector2(3., 2.5), start, dir, true);

  BOOST_CHECK(!solution.isValid());
}

/// Unit test for creating Ellipse intersection
BOOST_AUTO_TEST_CASE(EllipseIntersection) {
  // Basic checks for ellipse
  basicChecks();

  // Specific checks for ellipse
  double radiusX = 450.;
  double radiusY = 275.;

  Vector2 start(-500., -300.);
  Vector2 direction = Vector2(10., 4.).normalized();

  auto solution = detail::IntersectionHelper2D::intersectEllipse(
      radiusX, radiusY, start, direction);

  // Numerically checked / per hand calculated
  BOOST_CHECK(solution[0].isValid());

  CHECK_CLOSE_ABS(solution[0].position().x(), -283.68, 0.01);
  CHECK_CLOSE_ABS(solution[0].position().y(), -213.47, 0.01);
  BOOST_CHECK_GT(solution[0].pathLength(), 0.);

  BOOST_CHECK(solution[1].isValid());

  CHECK_CLOSE_ABS(solution[1].position().x(), 433.65, 0.01);
  CHECK_CLOSE_ABS(solution[1].position().y(), 73.46, 0.01);
  BOOST_CHECK_GT(solution[1].pathLength(), 0.);

  // Reverse checks will be done with circle (same code)
}

/// Unit test for creating Circle intersection
BOOST_AUTO_TEST_CASE(CircleIntersection) {
  // Basic checks for circle
  basicChecks(true);

  // Specific checks for circle
  double radius = 275.;

  Vector2 start(-500., -300.);
  Vector2 direction = Vector2(1., 1.).normalized();

  auto solution =
      detail::IntersectionHelper2D::intersectCircle(radius, start, direction);

  // Numerically checked / per hand calculated
  BOOST_CHECK(solution[0].isValid());

  CHECK_CLOSE_ABS(solution[0].position().x(), -266.771, 0.001);
  CHECK_CLOSE_ABS(solution[0].position().y(), -66.771, 0.001);
  BOOST_CHECK_GT(solution[0].pathLength(), 0.);

  BOOST_CHECK(solution[1].isValid());

  CHECK_CLOSE_ABS(solution[1].position().x(), 66.771, 0.001);
  CHECK_CLOSE_ABS(solution[1].position().y(), 266.771, 0.001);
  BOOST_CHECK_GT(solution[1].pathLength(), 0.);

  // Reverse
  start = Vector2(1500., 1700.);
  direction = Vector2(1., 1.).normalized();
  solution =
      detail::IntersectionHelper2D::intersectCircle(radius, start, direction);

  BOOST_CHECK(solution[0].isValid());
  CHECK_CLOSE_ABS(solution[0].position().x(), 66.771, 0.001);
  CHECK_CLOSE_ABS(solution[0].position().y(), 266.771, 0.001);
  BOOST_CHECK_LT(solution[0].pathLength(), 0.);

  BOOST_CHECK(solution[1].isValid());
  CHECK_CLOSE_ABS(solution[1].position().x(), -266.771, 0.001);
  CHECK_CLOSE_ABS(solution[1].position().y(), -66.771, 0.001);
  BOOST_CHECK_LT(solution[1].pathLength(), 0.);

  // Reverse with reverse direction
  direction = Vector2(-1., -1.).normalized();
  solution =
      detail::IntersectionHelper2D::intersectCircle(radius, start, direction);

  BOOST_CHECK(solution[0].isValid());
  CHECK_CLOSE_ABS(solution[0].position().x(), 66.771, 0.001);
  CHECK_CLOSE_ABS(solution[0].position().y(), 266.771, 0.001);
  BOOST_CHECK_GT(solution[0].pathLength(), 0.);

  BOOST_CHECK(solution[1].isValid());
  CHECK_CLOSE_ABS(solution[1].position().x(), -266.771, 0.001);
  CHECK_CLOSE_ABS(solution[1].position().y(), -66.771, 0.001);
  BOOST_CHECK_GT(solution[1].pathLength(), 0.);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
