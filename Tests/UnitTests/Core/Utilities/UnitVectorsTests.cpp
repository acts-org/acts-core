// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/UnitVectors.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

using Acts::Vector3;

namespace {
constexpr auto eps = std::numeric_limits<double>::epsilon();
}

BOOST_AUTO_TEST_SUITE(UnitVectors)

BOOST_AUTO_TEST_CASE(DirectionPhiEta) {
  using Acts::makeDirectionFromPhiEta;

  // along positive x
  const auto xPos1 = makeDirectionFromPhiEta(0.0, 0.0);
  CHECK_CLOSE_REL(xPos1.norm(), 1, eps);
  CHECK_CLOSE_REL(xPos1.dot(Vector3(1, 0, 0)), 1, eps);
  const auto xPos2 = makeDirectionFromPhiEta(2 * M_PI, 0.0);
  CHECK_CLOSE_REL(xPos2.norm(), 1, eps);
  CHECK_CLOSE_REL(xPos2.dot(Vector3(1, 0, 0)), 1, eps);
  // along negative x
  const auto xNeg1 = makeDirectionFromPhiEta(M_PI, 0.0);
  CHECK_CLOSE_REL(xNeg1.norm(), 1, eps);
  CHECK_CLOSE_REL(xNeg1.dot(Vector3(-1, 0, 0)), 1, eps);
  const auto xNeg2 = makeDirectionFromPhiEta(-M_PI, 0.0);
  CHECK_CLOSE_REL(xNeg2.norm(), 1, eps);
  CHECK_CLOSE_REL(xNeg2.dot(Vector3(-1, 0, 0)), 1, eps);
  // along positive y
  const auto yPos1 = makeDirectionFromPhiEta(M_PI_2, 0.0);
  CHECK_CLOSE_REL(yPos1.norm(), 1, eps);
  CHECK_CLOSE_REL(yPos1.dot(Vector3(0, 1, 0)), 1, eps);
  const auto yPos2 = makeDirectionFromPhiEta(-3 * M_PI_2, 0.0);
  CHECK_CLOSE_REL(yPos2.norm(), 1, eps);
  CHECK_CLOSE_REL(yPos2.dot(Vector3(0, 1, 0)), 1, eps);
  // along negative y
  const auto yNeg1 = makeDirectionFromPhiEta(-M_PI_2, 0.0);
  CHECK_CLOSE_REL(yNeg1.norm(), 1, eps);
  CHECK_CLOSE_REL(yNeg1.dot(Vector3(0, -1, 0)), 1, eps);
  const auto yNeg2 = makeDirectionFromPhiEta(3 * M_PI_2, 0.0);
  CHECK_CLOSE_REL(yNeg2.norm(), 1, eps);
  CHECK_CLOSE_REL(yNeg2.dot(Vector3(0, -1, 0)), 1, eps);

  const auto inf = std::numeric_limits<double>::infinity();
  // along positive z
  const auto zPos1 = makeDirectionFromPhiEta(0.0, inf);
  CHECK_CLOSE_REL(zPos1.norm(), 1, eps);
  CHECK_CLOSE_REL(zPos1.dot(Vector3(0, 0, 1)), 1, eps);
  const auto zPos2 = makeDirectionFromPhiEta(M_PI_2, inf);
  CHECK_CLOSE_REL(zPos2.norm(), 1, eps);
  CHECK_CLOSE_REL(zPos2.dot(Vector3(0, 0, 1)), 1, eps);
  // along negative z
  const auto zNeg1 = makeDirectionFromPhiEta(0.0, -inf);
  CHECK_CLOSE_REL(zNeg1.norm(), 1, eps);
  CHECK_CLOSE_REL(zNeg1.dot(Vector3(0, 0, -1)), 1, eps);
  const auto zNeg2 = makeDirectionFromPhiEta(M_PI_2, -inf);
  CHECK_CLOSE_REL(zNeg2.norm(), 1, eps);
  CHECK_CLOSE_REL(zNeg2.dot(Vector3(0, 0, -1)), 1, eps);

  // mixed direction
  const auto mixed1 = makeDirectionFromPhiEta(M_PI_4, 1.0);
  CHECK_CLOSE_REL(mixed1.norm(), 1, eps);
  CHECK_CLOSE_REL(
      mixed1.dot(Vector3(1, 1, M_SQRT2 * std::sinh(1.0)).normalized()), 1, eps);
  const auto mixed2 = makeDirectionFromPhiEta(M_PI_4, -1.0);
  CHECK_CLOSE_REL(mixed2.norm(), 1, eps);
  CHECK_CLOSE_REL(
      mixed2.dot(Vector3(1, 1, M_SQRT2 * std::sinh(-1.0)).normalized()), 1,
      eps);
  const auto mixed3 = makeDirectionFromPhiEta(-M_PI_4, -1.0);
  CHECK_CLOSE_REL(mixed3.norm(), 1, eps);
  CHECK_CLOSE_REL(
      mixed3.dot(Vector3(1, -1, M_SQRT2 * std::sinh(-1.0)).normalized()), 1,
      eps);
}

BOOST_AUTO_TEST_CASE(DirectionPhiTheta) {
  using Acts::makeDirectionFromPhiTheta;

  // along positive x
  const auto xPos1 = makeDirectionFromPhiTheta(0.0, M_PI_2);
  CHECK_CLOSE_REL(xPos1.norm(), 1, eps);
  CHECK_CLOSE_REL(xPos1.dot(Vector3(1, 0, 0)), 1, eps);
  const auto xPos2 = makeDirectionFromPhiTheta(2 * M_PI, M_PI_2);
  CHECK_CLOSE_REL(xPos2.norm(), 1, eps);
  CHECK_CLOSE_REL(xPos2.dot(Vector3(1, 0, 0)), 1, eps);
  // along negative x
  const auto xNeg1 = makeDirectionFromPhiTheta(M_PI, M_PI_2);
  CHECK_CLOSE_REL(xNeg1.norm(), 1, eps);
  CHECK_CLOSE_REL(xNeg1.dot(Vector3(-1, 0, 0)), 1, eps);
  const auto xNeg2 = makeDirectionFromPhiTheta(-M_PI, M_PI_2);
  CHECK_CLOSE_REL(xNeg2.norm(), 1, eps);
  CHECK_CLOSE_REL(xNeg2.dot(Vector3(-1, 0, 0)), 1, eps);
  // along positive y
  const auto yPos1 = makeDirectionFromPhiTheta(M_PI_2, M_PI_2);
  CHECK_CLOSE_REL(yPos1.norm(), 1, eps);
  CHECK_CLOSE_REL(yPos1.dot(Vector3(0, 1, 0)), 1, eps);
  const auto yPos2 = makeDirectionFromPhiTheta(-3 * M_PI_2, M_PI_2);
  CHECK_CLOSE_REL(yPos2.norm(), 1, eps);
  CHECK_CLOSE_REL(yPos2.dot(Vector3(0, 1, 0)), 1, eps);
  // along negative y
  const auto yNeg1 = makeDirectionFromPhiTheta(-M_PI_2, M_PI_2);
  CHECK_CLOSE_REL(yNeg1.norm(), 1, eps);
  CHECK_CLOSE_REL(yNeg1.dot(Vector3(0, -1, 0)), 1, eps);
  const auto yNeg2 = makeDirectionFromPhiTheta(3 * M_PI_2, M_PI_2);
  CHECK_CLOSE_REL(yNeg2.norm(), 1, eps);
  CHECK_CLOSE_REL(yNeg2.dot(Vector3(0, -1, 0)), 1, eps);

  // along positive z
  const auto zPos1 = makeDirectionFromPhiTheta(0.0, 0.0);
  CHECK_CLOSE_REL(zPos1.norm(), 1, eps);
  CHECK_CLOSE_REL(zPos1.dot(Vector3(0, 0, 1)), 1, eps);
  const auto zPos2 = makeDirectionFromPhiTheta(M_PI_2, 0.0);
  CHECK_CLOSE_REL(zPos2.norm(), 1, eps);
  CHECK_CLOSE_REL(zPos2.dot(Vector3(0, 0, 1)), 1, eps);
  // along negative z
  const auto zNeg1 = makeDirectionFromPhiTheta(0.0, M_PI);
  CHECK_CLOSE_REL(zNeg1.norm(), 1, eps);
  CHECK_CLOSE_REL(zNeg1.dot(Vector3(0, 0, -1)), 1, eps);
  const auto zNeg2 = makeDirectionFromPhiTheta(M_PI_2, M_PI);
  CHECK_CLOSE_REL(zNeg2.norm(), 1, eps);
  CHECK_CLOSE_REL(zNeg2.dot(Vector3(0, 0, -1)), 1, eps);

  // mixed direction
  const auto mixed1 = makeDirectionFromPhiTheta(M_PI_4, M_PI_4);
  CHECK_CLOSE_REL(mixed1.norm(), 1, eps);
  CHECK_CLOSE_REL(mixed1.dot(Vector3(1, 1, M_SQRT2).normalized()), 1, eps);
  const auto mixed2 = makeDirectionFromPhiTheta(M_PI_4, 3 * M_PI_4);
  CHECK_CLOSE_REL(mixed2.norm(), 1, eps);
  CHECK_CLOSE_REL(mixed2.dot(Vector3(1, 1, -M_SQRT2).normalized()), 1, eps);
  const auto mixed3 = makeDirectionFromPhiTheta(-M_PI_4, 3 * M_PI_4);
  CHECK_CLOSE_REL(mixed3.norm(), 1, eps);
  CHECK_CLOSE_REL(mixed3.dot(Vector3(1, -1, -M_SQRT2).normalized()), 1, eps);
}

namespace {
template <typename Direction, typename RefUnitU, typename RefUnitV>
void testCurvilinear(const Eigen::MatrixBase<Direction>& direction,
                     const Eigen::MatrixBase<RefUnitU>& refU,
                     const Eigen::MatrixBase<RefUnitV>& refV) {
  const auto u = Acts::makeCurvilinearUnitU(direction);
  const auto uv = Acts::makeCurvilinearUnitVectors(direction);
  // verify normalization
  CHECK_CLOSE_ABS(u.norm(), 1, eps);
  CHECK_CLOSE_ABS(uv.first.norm(), 1, eps);
  CHECK_CLOSE_ABS(uv.second.norm(), 1, eps);
  // verify orthonormality
  CHECK_SMALL(u.dot(direction), eps);
  CHECK_SMALL(uv.first.dot(direction), eps);
  CHECK_SMALL(uv.second.dot(direction), eps);
  CHECK_SMALL(uv.first.dot(uv.second), eps);
  // verify u is in the x-y plane
  CHECK_SMALL(u[2], eps);
  CHECK_SMALL(uv.first[2], eps);
  // verify references. do not use element-wise comparison to avoid issues with
  // small, epsilon-like differences.
  CHECK_CLOSE_ABS(u.dot(refU), 1, eps);
  CHECK_CLOSE_ABS(uv.first.dot(refU), 1, eps);
  CHECK_CLOSE_ABS(uv.second.dot(refV), 1, eps);
}
}  // namespace

BOOST_AUTO_TEST_CASE(CurvilinearTransverse) {
  // curvilinear system w/ direction in the transverse plane
  testCurvilinear(Vector3(1, 1, 0), Vector3(-1, 1, 0).normalized(),
                  Vector3(0, 0, 1).normalized());
}

BOOST_AUTO_TEST_CASE(CurvilinearPositiveZ) {
  // curvilinear system w/ direction along z
  testCurvilinear(Vector3(0, 0, 1), Vector3(1, 0, 0), Vector3(0, 1, 0));
}

BOOST_AUTO_TEST_CASE(CurvilinearNegativeZ) {
  // curvilinear system w/ direction along z
  testCurvilinear(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0));
}

BOOST_AUTO_TEST_CASE(CurvilinearCloseToZ) {
  // curvilinear system w/ direction close to z
  testCurvilinear(Vector3(0, 32 * eps, 1 - 32 * eps), Vector3(-1, 0, 0),
                  Vector3(0, -1, 32 * eps));
}

BOOST_AUTO_TEST_SUITE_END()
