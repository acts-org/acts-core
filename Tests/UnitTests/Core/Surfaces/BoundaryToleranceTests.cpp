// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <limits>
#include <optional>
#include <random>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

BOOST_AUTO_TEST_CASE(BoundaryToleranceConstructors) {
  using enum BoundaryTolerance::ToleranceMode;
  {
    // Test None constructor
    BoundaryTolerance tolerance = BoundaryTolerance::None();
    BOOST_CHECK(tolerance.toleranceMode() == None);
  }

  // Test AbsoluteBound constructor
  {
    // Valid positive tolerances
    auto tolerance = BoundaryTolerance::AbsoluteBound(1.0, 2.0);
    BOOST_CHECK_EQUAL(tolerance.asAbsoluteBound().tolerance0, 1.0);
    BOOST_CHECK_EQUAL(tolerance.asAbsoluteBound().tolerance1, 2.0);
    BOOST_CHECK(tolerance.toleranceMode() == Extend);
    BOOST_CHECK(BoundaryTolerance::AbsoluteBound(0.0, 0.0).toleranceMode() ==
                None);

    // Negative tolerances should throw
    BOOST_CHECK_THROW(BoundaryTolerance::AbsoluteBound(-1.0, 2.0),
                      std::invalid_argument);
    BOOST_CHECK_THROW(BoundaryTolerance::AbsoluteBound(1.0, -2.0),
                      std::invalid_argument);
  }

  // Test AbsoluteEuclidean constructor
  {
    // Valid positive tolerance
    auto tolerance = BoundaryTolerance::AbsoluteEuclidean(1.0);
    BOOST_CHECK_EQUAL(tolerance.asAbsoluteEuclidean().tolerance, 1.0);
    BOOST_CHECK(tolerance.toleranceMode() == Extend);
    BOOST_CHECK(BoundaryTolerance::AbsoluteEuclidean(0.0).toleranceMode() ==
                None);

    // Valid negative tolerance
    tolerance = BoundaryTolerance::AbsoluteEuclidean(-1.0);
    BOOST_CHECK_EQUAL(tolerance.asAbsoluteEuclidean().tolerance, -1.0);
    BOOST_CHECK(tolerance.toleranceMode() == Shrink);
  }

  // Test AbsoluteCartesian constructor
  {
    // Valid positive tolerance
    auto tolerance = BoundaryTolerance::AbsoluteCartesian(1.0, 2.0);
    BOOST_CHECK_EQUAL(tolerance.asAbsoluteCartesian().tolerance0, 1.0);
    BOOST_CHECK_EQUAL(tolerance.asAbsoluteCartesian().tolerance1, 2.0);
    BOOST_CHECK(tolerance.toleranceMode() == Extend);
    BOOST_CHECK(
        BoundaryTolerance::AbsoluteCartesian(0.0, 0.0).toleranceMode() == None);

    // Negative tolerances should throw
    BOOST_CHECK_THROW(BoundaryTolerance::AbsoluteCartesian(-1.0, 2.0),
                      std::invalid_argument);
    BOOST_CHECK_THROW(BoundaryTolerance::AbsoluteCartesian(1.0, -2.0),
                      std::invalid_argument);
  }

  // Test Chi2Bound constructor
  {
    SquareMatrix2 cov;
    cov << 1, 0.5, 0.5, 2;

    // Valid positive chi2 bound
    auto tolerance = BoundaryTolerance::Chi2Bound(cov, 3.0);
    BOOST_CHECK_EQUAL(tolerance.asChi2Bound().maxChi2, 3.0);
    BOOST_CHECK(tolerance.toleranceMode() == Extend);
    BOOST_CHECK(BoundaryTolerance::Chi2Bound(cov, 0.0).toleranceMode() == None);

    // Valid negative chi2 bound
    tolerance = BoundaryTolerance::Chi2Bound(cov, -3.0);
    BOOST_CHECK_EQUAL(tolerance.asChi2Bound().maxChi2, -3.0);
    BOOST_CHECK(tolerance.toleranceMode() == Shrink);
  }

  // Test None constructor
  BoundaryTolerance::None();
}

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
  auto tolerance = BoundaryTolerance::Chi2Bound(cov.inverse(), 3.);
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
        BoundaryTolerance::Chi2Bound(SquareMatrix2::Identity(), 1.);
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

BOOST_AUTO_TEST_CASE(BoundaryCheckNegativeToleranceRect) {
  // Test points for boundary check with euclidean tolerance
  Vector2 ll(1, 1);
  Vector2 ur(3, 3);

  auto check = [&ll, &ur](const BoundaryTolerance& tolerance,
                          const Vector2& point) {
    return detail::insideAlignedBox(ll, ur, tolerance, point, std::nullopt);
  };

  {
    auto tolerance = BoundaryTolerance::AbsoluteEuclidean(-0.25);

    BOOST_CHECK(!check(tolerance, {2.8, 2}));
    BOOST_CHECK(!check(tolerance, {3.1, 2}));
    BOOST_CHECK(check(tolerance, {2.7, 2}));
    BOOST_CHECK(!check(tolerance, {2, 3.1}));
    BOOST_CHECK(!check(tolerance, {2, 2.8}));
    BOOST_CHECK(check(tolerance, {2, 2.7}));

    BOOST_CHECK(!check(tolerance, {0.8, 2}));
    BOOST_CHECK(!check(tolerance, {1.2, 2}));
    BOOST_CHECK(check(tolerance, {1.5, 2}));
    BOOST_CHECK(!check(tolerance, {2, 0.8}));
    BOOST_CHECK(!check(tolerance, {2, 1.2}));
    BOOST_CHECK(check(tolerance, {2, 1.5}));
  }

  {
    auto tolerance =
        BoundaryTolerance::Chi2Bound(SquareMatrix2::Identity(), -0.1);

    BOOST_CHECK(!check(tolerance, {2.8, 2}));
    BOOST_CHECK(!check(tolerance, {3.1, 2}));
    BOOST_CHECK(check(tolerance, {2.5, 2}));
    BOOST_CHECK(!check(tolerance, {2, 3.1}));
    BOOST_CHECK(!check(tolerance, {2, 2.8}));
    BOOST_CHECK(check(tolerance, {2, 2.5}));

    BOOST_CHECK(!check(tolerance, {0.8, 2}));
    BOOST_CHECK(!check(tolerance, {1.4, 2}));
    BOOST_CHECK(check(tolerance, {1.5, 2}));
    BOOST_CHECK(!check(tolerance, {2, 0.8}));
    BOOST_CHECK(!check(tolerance, {2, 1.4}));
    BOOST_CHECK(check(tolerance, {2, 1.5}));
  }
}

BOOST_AUTO_TEST_CASE(BoundaryCheckNegativeToleranceTrap) {
  Vector2 vertices[] = {{1.5, 1}, {2.5, 1}, {3, 3}, {1, 3}};

  auto check = [&vertices](const BoundaryTolerance& tolerance,
                           const Vector2& point) {
    return detail::insidePolygon(vertices, tolerance, point, std::nullopt);
  };

  {
    auto tolerance = BoundaryTolerance::AbsoluteEuclidean(0.25);
    // Axes
    BOOST_CHECK(!check(tolerance, {3.1, 2}));
    BOOST_CHECK(check(tolerance, {2.75, 2}));
    BOOST_CHECK(check(tolerance, {2.5, 2}));
    BOOST_CHECK(check(tolerance, {2.25, 2}));
    BOOST_CHECK(check(tolerance, {2, 3.1}));
    BOOST_CHECK(check(tolerance, {2, 2.75}));
    BOOST_CHECK(check(tolerance, {2, 2.5}));
    BOOST_CHECK(check(tolerance, {2, 2.25}));
    BOOST_CHECK(check(tolerance, {2, 2}));

    // Corners
    BOOST_CHECK(check(tolerance, {3.1, 3.2}));
    BOOST_CHECK(check(tolerance, {0.9, 3.2}));
    BOOST_CHECK(check(tolerance, {1.5, 0.8}));
    BOOST_CHECK(check(tolerance, {2.5, 0.8}));
  }

  {
    auto tolerance = BoundaryTolerance::AbsoluteEuclidean(-0.25);
    // Axes
    BOOST_CHECK(!check(tolerance, {3.0, 2}));
    BOOST_CHECK(!check(tolerance, {2.5, 2}));
    BOOST_CHECK(check(tolerance, {2.25, 2}));
    BOOST_CHECK(!check(tolerance, {2, 3.1}));
    BOOST_CHECK(!check(tolerance, {2, 2.9}));
    BOOST_CHECK(check(tolerance, {2, 2.7}));

    // Corners
    BOOST_CHECK(!check(tolerance, {2.7, 2.9}));
    BOOST_CHECK(check(tolerance, {2.4, 2.6}));
    BOOST_CHECK(!check(tolerance, {1.3, 2.9}));
    BOOST_CHECK(check(tolerance, {1.6, 2.6}));
    BOOST_CHECK(!check(tolerance, {2.4, 1.1}));
    BOOST_CHECK(check(tolerance, {1.75, 1.4}));
    BOOST_CHECK(!check(tolerance, {1.6, 1.1}));
    BOOST_CHECK(check(tolerance, {2.25, 1.4}));
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
