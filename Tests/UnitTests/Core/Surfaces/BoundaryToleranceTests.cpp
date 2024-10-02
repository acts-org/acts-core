// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

#include "BoundaryToleranceTestsRefs.hpp"

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

// See: https://en.wikipedia.org/wiki/Bounding_volume
//
// Aligned box w/ simple check
BOOST_AUTO_TEST_CASE(BoundaryCheckBoxSimple) {
  Vector2 ll(-1, -1);
  Vector2 ur(1, 1);
  auto tolerance = BoundaryTolerance::None();
  BOOST_CHECK(
      detail::insideAlignedBox(ll, ur, tolerance, {0, 0}, std::nullopt));
  BOOST_CHECK(
      !detail::insideAlignedBox(ll, ur, tolerance, {2, 2}, std::nullopt));
  BOOST_CHECK(
      !detail::insideAlignedBox(ll, ur, tolerance, {0, 2}, std::nullopt));
  BOOST_CHECK(
      !detail::insideAlignedBox(ll, ur, tolerance, {2, 0}, std::nullopt));
}
// Aligned box w/ tolerance check along first axis
BOOST_AUTO_TEST_CASE(BoundaryCheckBoxToleranceLoc0) {
  boost::execution_monitor em;
  em.p_detect_fp_exceptions.set(boost::fpe::BOOST_FPE_ALL);

  em.execute([]() {
    Vector2 ll(-1, -1);
    Vector2 ur(1, 1);
    auto tolerance = BoundaryTolerance::AbsoluteBound(
        1.5, std::numeric_limits<double>::infinity());
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 0}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {2, 2}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {4, 4}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 2}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {2, 0}, std::nullopt));

    return 0;
  });
}

// Aligned box w/ covariance check
BOOST_AUTO_TEST_CASE(BoundaryCheckBoxCovariance) {
  SquareMatrix2 cov;
  cov << 1, 0.5, 0.5, 2;
  Vector2 ll(-1, -1);
  Vector2 ur(1, 1);
  auto tolerance = BoundaryTolerance::Chi2Bound(cov.inverse(), 3.0);
  BOOST_CHECK(
      detail::insideAlignedBox(ll, ur, tolerance, {0, 0}, std::nullopt));
  BOOST_CHECK(
      detail::insideAlignedBox(ll, ur, tolerance, {2, 2}, std::nullopt));
  BOOST_CHECK(
      !detail::insideAlignedBox(ll, ur, tolerance, {4, 4}, std::nullopt));
  BOOST_CHECK(
      detail::insideAlignedBox(ll, ur, tolerance, {0, 3}, std::nullopt));
  BOOST_CHECK(
      detail::insideAlignedBox(ll, ur, tolerance, {3, 0}, std::nullopt));
}

// Triangle w/ simple check
BOOST_AUTO_TEST_CASE(BoundaryCheckTriangleSimple) {
  Vector2 vertices[] = {{-2, 0}, {2, 0}, {0, 2}};
  auto tolerance = BoundaryTolerance::None();
  BOOST_CHECK(detail::insidePolygon(vertices, tolerance, {0, 0}, std::nullopt));
  BOOST_CHECK(detail::insidePolygon(vertices, tolerance, {0, 1}, std::nullopt));
  BOOST_CHECK(
      !detail::insidePolygon(vertices, tolerance, {2, 2}, std::nullopt));
  BOOST_CHECK(
      !detail::insidePolygon(vertices, tolerance, {0, -1}, std::nullopt));
}
// Triangle w/ covariance check
BOOST_AUTO_TEST_CASE(BoundaryCheckTriangleCovariance) {
  Vector2 vertices[] = {{-2, 0}, {2, 0}, {0, 2}};
  SquareMatrix2 cov;
  cov << 0.5, 0, 0, 0.5;
  auto tolerance = BoundaryTolerance::Chi2Bound(cov.inverse(), 4.1);
  BOOST_CHECK(detail::insidePolygon(vertices, tolerance, {0, 0}, std::nullopt));
  BOOST_CHECK(detail::insidePolygon(vertices, tolerance, {0, 1}, std::nullopt));
  BOOST_CHECK(detail::insidePolygon(vertices, tolerance, {0, 2}, std::nullopt));
  BOOST_CHECK(detail::insidePolygon(vertices, tolerance, {0, 3}, std::nullopt));
  BOOST_CHECK(detail::insidePolygon(vertices, tolerance, {0, 4}, std::nullopt));
  BOOST_CHECK(
      !detail::insidePolygon(vertices, tolerance, {0, 5}, std::nullopt));
}

BOOST_AUTO_TEST_CASE(BoundaryCheckDifferentTolerances) {
  Vector2 ll(-1, -1);
  Vector2 ur(1, 1);

  {
    auto tolerance = BoundaryTolerance::None();
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 0}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {2, 2}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {0, 2}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {2, 0}, std::nullopt));
  }

  {
    auto tolerance = BoundaryTolerance::Infinite();
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 0}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {2, 2}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 2}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {2, 0}, std::nullopt));
  }

  {
    auto tolerance = BoundaryTolerance::AbsoluteBound(0.5, 0.5);
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 0}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {1.1, 1.1}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 1.1}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {1.1, 0}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {2, 2}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {0, 2}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {2, 0}, std::nullopt));
  }

  {
    auto tolerance = BoundaryTolerance::AbsoluteCartesian(0.5, 0.5);
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 0}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {1.1, 1.1}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 1.1}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {1.1, 0}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {2, 2}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {0, 2}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {2, 0}, std::nullopt));
  }

  {
    auto tolerance = BoundaryTolerance::AbsoluteEuclidean(1.1);
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 0}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {2, 2}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 2}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {2, 0}, std::nullopt));
  }

  {
    auto tolerance =
        BoundaryTolerance::Chi2Bound(SquareMatrix2::Identity(), 1.0);
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 0}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {2, 2}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {0, 2}, std::nullopt));
    BOOST_CHECK(
        detail::insideAlignedBox(ll, ur, tolerance, {2, 0}, std::nullopt));
    BOOST_CHECK(
        !detail::insideAlignedBox(ll, ur, tolerance, {3, 3}, std::nullopt));
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
