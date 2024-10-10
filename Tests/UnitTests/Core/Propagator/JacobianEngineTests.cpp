// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Propagator/detail/JacobianEngine.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"

namespace Acts::Test {

/// These tests do not test for a correct covariance transport but only for the
/// correct conservation or modification of certain variables. A test suite for
/// the numerical correctness is performed in the integration tests.
///
BOOST_AUTO_TEST_CASE(jacobian_engine_to_bound) {
  // Create a test context
  GeometryContext tgContext = GeometryContext();

  // Build a start vector
  Vector3 position{1., 2., 3.};
  double time = 4.;
  Vector3 direction = Vector3(5., 2., 7.).normalized();
  double qop = 0.125;

  // Build a surface
  auto pSurface = CurvilinearSurface(position, direction).planeSurface();

  // Other rotated surface
  Vector3 odirection = Vector3(6., 2., 8.).normalized();
  auto oSurface = CurvilinearSurface(position, odirection).planeSurface();

  // The free parameter vector
  FreeVector freeParameters;
  freeParameters << position[0], position[1], position[2], time, direction[0],
      direction[1], direction[2], qop;
  // And its associated bound vector
  BoundVector boundParameters;
  boundParameters << 0., 0., VectorHelpers::phi(direction),
      VectorHelpers::theta(direction), qop, time;

  // Build covariance matrices for bound and free case
  BoundSquareMatrix boundCovariance = 0.025 * BoundSquareMatrix::Identity();
  FreeSquareMatrix freeCovariance = 0.025 * FreeSquareMatrix::Identity();

  FreeMatrix noTransportJacobian = FreeMatrix::Identity();
  FreeMatrix realTransportJacobian = 2 * FreeMatrix::Identity();

  FreeToPathMatrix freeToPathDerivatives =
      pSurface->freeToPathDerivative(tgContext, position, direction);
  BoundToFreeMatrix boundToFreeJacobian =
      pSurface->boundToFreeJacobian(tgContext, position, direction);

  // (1) curvilinear/bound to bound transport jacobian
  // a) test without actual transport into the same surface
  BoundMatrix b2bTransportJacobian;
  detail::boundToBoundTransportJacobian(
      tgContext, *pSurface, freeParameters, boundToFreeJacobian,
      noTransportJacobian, freeToPathDerivatives, b2bTransportJacobian);
  BoundSquareMatrix newBoundCovariance =
      b2bTransportJacobian * boundCovariance * b2bTransportJacobian.transpose();
  BOOST_CHECK(boundCovariance.isApprox(newBoundCovariance));
  // b) test without actual transport but to a new surface
  detail::boundToBoundTransportJacobian(
      tgContext, *oSurface, freeParameters, boundToFreeJacobian,
      noTransportJacobian, freeToPathDerivatives, b2bTransportJacobian);
  newBoundCovariance =
      b2bTransportJacobian * boundCovariance * b2bTransportJacobian.transpose();
  BOOST_CHECK(!boundCovariance.isApprox(newBoundCovariance));
  // c) test to "the same" surface with transport
  // (not really senseful, but should give a different result)
  detail::boundToBoundTransportJacobian(
      tgContext, *pSurface, freeParameters, boundToFreeJacobian,
      realTransportJacobian, freeToPathDerivatives, b2bTransportJacobian);
  newBoundCovariance =
      b2bTransportJacobian * boundCovariance * b2bTransportJacobian.transpose();
  BOOST_CHECK(!boundCovariance.isApprox(newBoundCovariance));

  FreeToBoundMatrix f2bTransportJacobian;
  detail::freeToBoundTransportJacobian(
      tgContext, *pSurface, freeParameters, noTransportJacobian,
      freeToPathDerivatives, f2bTransportJacobian);

  newBoundCovariance =
      f2bTransportJacobian * freeCovariance * f2bTransportJacobian.transpose();
  BOOST_CHECK(!boundCovariance.isApprox(newBoundCovariance));
}

/// These tests do not test for a correct covariance transport but only for the
/// correct conservation or modification of certain variables. A test suite for
/// the numerical correctness is performed in the integration tests.
///
BOOST_AUTO_TEST_CASE(jacobian_engine_to_curvilinear) {
  // Create a test context
  GeometryContext tgContext = GeometryContext();

  // Build a start vector
  Vector3 position{1., 2., 3.};
  Vector3 direction = Vector3(5., 2., 7.).normalized();

  // Build a surface, starting surface for curvilinear
  auto pSurface = CurvilinearSurface(position, direction).planeSurface();

  // Build covariance matrices for bound and free case
  BoundSquareMatrix boundCovariance = 0.025 * BoundSquareMatrix::Identity();
  FreeSquareMatrix freeCovariance = 0.025 * FreeSquareMatrix::Identity();

  FreeMatrix noTransportJacobian = FreeMatrix::Identity();

  FreeToPathMatrix freeToPathDerivatives =
      pSurface->freeToPathDerivative(tgContext, position, direction);
  BoundToFreeMatrix boundToFreeJacobian =
      CurvilinearSurface(direction).boundToFreeJacobian();

  // (1) curvilinear/bound to curvilinear transport jacobian
  // a) test without actual transport into the same surface
  BoundMatrix b2cTransportJacobian;
  detail::boundToCurvilinearTransportJacobian(
      direction, boundToFreeJacobian, noTransportJacobian,
      freeToPathDerivatives, b2cTransportJacobian);
  BoundSquareMatrix newBoundCovariance =
      b2cTransportJacobian * boundCovariance * b2cTransportJacobian.transpose();
  BOOST_CHECK(boundCovariance.isApprox(newBoundCovariance));
  // b) test to another curvilinear frame at the same point (no transport)
  detail::boundToCurvilinearTransportJacobian(
      Vector3(4., 5., 6.).normalized(), boundToFreeJacobian,
      noTransportJacobian, freeToPathDerivatives, b2cTransportJacobian);
  newBoundCovariance =
      b2cTransportJacobian * boundCovariance * b2cTransportJacobian.transpose();
  BOOST_CHECK(!boundCovariance.isApprox(newBoundCovariance));

  FreeToBoundMatrix f2cTransportJacobian =
      detail::freeToCurvilinearTransportJacobian(direction, noTransportJacobian,
                                                 freeToPathDerivatives);

  newBoundCovariance =
      f2cTransportJacobian * freeCovariance * f2cTransportJacobian.transpose();
  BOOST_CHECK(!boundCovariance.isApprox(newBoundCovariance));
}

/// These tests do not test for a correct covariance transport but only for the
/// correct conservation or modification of certain variables. A test suite for
/// the numerical correctness is performed in the integration tests.
///
BOOST_AUTO_TEST_CASE(jacobian_engine_to_free) {
  // Create a test context
  GeometryContext tgContext = GeometryContext();

  // Build a start vector
  Vector3 position{1., 2., 3.};
  Vector3 direction = Vector3(5., 2., 7.).normalized();

  // Build a surface, starting surface for curvilinear
  auto pSurface = CurvilinearSurface(position, direction).planeSurface();

  // Build covariance matrices for bound and free case
  BoundSquareMatrix boundCovariance = 0.025 * BoundSquareMatrix::Identity();
  FreeSquareMatrix freeCovariance = 0.025 * FreeSquareMatrix::Identity();

  FreeMatrix noTransportJacobian = FreeMatrix::Identity();

  BoundToFreeMatrix boundToFreeJacobian =
      pSurface->boundToFreeJacobian(tgContext, position, direction);

  // (1) bound to free
  BoundToFreeMatrix b2fTransportJacobian = detail::boundToFreeTransportJacobian(
      boundToFreeJacobian, noTransportJacobian);

  FreeMatrix newFreeCovariance1 =
      b2fTransportJacobian * boundCovariance * b2fTransportJacobian.transpose();
  BOOST_CHECK(!newFreeCovariance1.isApprox(freeCovariance));

  // (2) curvilinear to free
  boundToFreeJacobian = CurvilinearSurface(direction).boundToFreeJacobian();
  BoundToFreeMatrix c2fTransportJacobian = detail::boundToFreeTransportJacobian(
      boundToFreeJacobian, noTransportJacobian);

  FreeMatrix newFreeCovariance2 =
      c2fTransportJacobian * boundCovariance * c2fTransportJacobian.transpose();
  BOOST_CHECK(!newFreeCovariance2.isApprox(freeCovariance));
  // But those should be similar/equal
  BOOST_CHECK(newFreeCovariance1.isApprox(newFreeCovariance2));
}

}  // namespace Acts::Test
