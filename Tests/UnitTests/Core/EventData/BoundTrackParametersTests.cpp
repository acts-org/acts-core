// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/ConeSurface.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <cmath>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>

#include "TrackParametersDatasets.hpp"

namespace {

namespace bdata = boost::unit_test::data;
using namespace Acts;
using namespace Acts::UnitLiterals;

constexpr auto eps = 8 * std::numeric_limits<double>::epsilon();
const GeometryContext geoCtx;
const BoundSquareMatrix cov = BoundSquareMatrix::Identity();

void checkParameters(const BoundTrackParameters& params, double l0, double l1,
                     double time, double phi, double theta, double p, double q,
                     const Vector3& pos, const Vector3& unitDir) {
  const auto particleHypothesis = ParticleHypothesis::pionLike(std::abs(q));

  const auto qOverP = particleHypothesis.qOverP(p, q);
  const auto pos4 = VectorHelpers::makeVector4(pos, time);

  // native values
  CHECK_CLOSE_OR_SMALL(params.template get<eBoundLoc0>(), l0, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eBoundLoc1>(), l1, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eBoundTime>(), time, eps, eps);
  CHECK_CLOSE_OR_SMALL(detail::radian_sym(params.template get<eBoundPhi>()),
                       detail::radian_sym(phi), eps, eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eBoundTheta>(), theta, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eBoundQOverP>(), qOverP, eps, eps);
  // convenience accessors
  CHECK_CLOSE_OR_SMALL(params.fourPosition(geoCtx), pos4, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.position(geoCtx), pos, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.time(), time, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.direction(), unitDir, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.absoluteMomentum(), p, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.transverseMomentum(), p * std::sin(theta), eps,
                       eps);
  CHECK_CLOSE_OR_SMALL(params.momentum(), p * unitDir, eps, eps);
  BOOST_CHECK_EQUAL(params.charge(), q);

  // reflection
  BoundTrackParameters reflectedParams = params;
  reflectedParams.reflectInPlace();
  CHECK_CLOSE_OR_SMALL(params.reflect().parameters(),
                       reflectedParams.parameters(), eps, eps);
  CHECK_CLOSE_OR_SMALL(reflectedParams.reflect().parameters(),
                       params.parameters(), eps, eps);
}

void runTest(const std::shared_ptr<const Surface>& surface, double l0,
             double l1, double time, double phi, double theta, double p) {
  // phi is ill-defined in forward/backward tracks
  phi = ((0 < theta) && (theta < std::numbers::pi)) ? phi : 0.;

  // global direction for reference
  const Vector3 dir = makeDirectionFromPhiTheta(phi, theta);
  // convert local-to-global for reference
  const Vector2 loc(l0, l1);
  const Vector3 pos = surface->localToGlobal(geoCtx, loc, dir);
  // global four-position as input
  Vector4 pos4;
  pos4.segment<3>(ePos0) = pos;
  pos4[eTime] = time;

  // neutral parameters from local vector
  {
    BoundVector vector = BoundVector::Zero();
    vector[eBoundLoc0] = l0;
    vector[eBoundLoc1] = l1;
    vector[eBoundTime] = time;
    vector[eBoundPhi] = phi;
    vector[eBoundTheta] = theta;
    vector[eBoundQOverP] = 1 / p;
    BoundTrackParameters params(surface, vector, std::nullopt,
                                ParticleHypothesis::pion0());
    checkParameters(params, l0, l1, time, phi, theta, p, 0_e, pos, dir);
    BOOST_CHECK(!params.covariance());

    // reassign w/ covariance
    params =
        BoundTrackParameters(surface, vector, cov, ParticleHypothesis::pion0());
    checkParameters(params, l0, l1, time, phi, theta, p, 0_e, pos, dir);
    BOOST_CHECK(params.covariance());
    BOOST_CHECK_EQUAL(params.covariance().value(), cov);
  }
  // negative charged parameters from local vector
  {
    BoundVector vector = BoundVector::Zero();
    vector[eBoundLoc0] = l0;
    vector[eBoundLoc1] = l1;
    vector[eBoundTime] = time;
    vector[eBoundPhi] = phi;
    vector[eBoundTheta] = theta;
    vector[eBoundQOverP] = -1_e / p;
    BoundTrackParameters params(surface, vector, std::nullopt,
                                ParticleHypothesis::pion());
    checkParameters(params, l0, l1, time, phi, theta, p, -1_e, pos, dir);
    BOOST_CHECK(!params.covariance());

    // reassign w/ covariance
    params =
        BoundTrackParameters(surface, vector, cov, ParticleHypothesis::pion());
    checkParameters(params, l0, l1, time, phi, theta, p, -1_e, pos, dir);
    BOOST_CHECK(params.covariance());
    BOOST_CHECK_EQUAL(params.covariance().value(), cov);
  }
  // positive charged parameters from local vector
  {
    BoundVector vector = BoundVector::Zero();
    vector[eBoundLoc0] = l0;
    vector[eBoundLoc1] = l1;
    vector[eBoundTime] = time;
    vector[eBoundPhi] = phi;
    vector[eBoundTheta] = theta;
    vector[eBoundQOverP] = 1_e / p;
    BoundTrackParameters params(surface, vector, std::nullopt,
                                ParticleHypothesis::pion());
    checkParameters(params, l0, l1, time, phi, theta, p, 1_e, pos, dir);
    BOOST_CHECK(!params.covariance());

    // reassign w/ covariance
    params =
        BoundTrackParameters(surface, vector, cov, ParticleHypothesis::pion());
    checkParameters(params, l0, l1, time, phi, theta, p, 1_e, pos, dir);
    BOOST_CHECK(params.covariance());
    BOOST_CHECK_EQUAL(params.covariance().value(), cov);
  }
  // double-negative charged any parameters from local vector
  {
    BoundVector vector = BoundVector::Zero();
    vector[eBoundLoc0] = l0;
    vector[eBoundLoc1] = l1;
    vector[eBoundTime] = time;
    vector[eBoundPhi] = phi;
    vector[eBoundTheta] = theta;
    vector[eBoundQOverP] = -2_e / p;
    BoundTrackParameters params(surface, vector, std::nullopt,
                                ParticleHypothesis::pionLike(2_e));
    checkParameters(params, l0, l1, time, phi, theta, p, -2_e, pos, dir);
    BOOST_CHECK(!params.covariance());

    // reassign w/ covariance
    params = BoundTrackParameters(surface, vector, cov,
                                  ParticleHypothesis::pionLike(2_e));
    checkParameters(params, l0, l1, time, phi, theta, p, -2_e, pos, dir);
    BOOST_CHECK(params.covariance());
    BOOST_CHECK_EQUAL(params.covariance().value(), cov);
  }
  // neutral parameters from global information
  {
    auto params =
        BoundTrackParameters::create(surface, geoCtx, pos4, dir, 1 / p,
                                     std::nullopt, ParticleHypothesis::pion0())
            .value();
    checkParameters(params, l0, l1, time, phi, theta, p, 0_e, pos, dir);
    BOOST_CHECK(!params.covariance());
  }
  // negative charged parameters from global information
  {
    auto params =
        BoundTrackParameters::create(surface, geoCtx, pos4, dir, -1_e / p,
                                     std::nullopt, ParticleHypothesis::pion())
            .value();
    checkParameters(params, l0, l1, time, phi, theta, p, -1_e, pos, dir);
    BOOST_CHECK(!params.covariance());
  }
  // positive charged parameters from global information
  {
    auto params =
        BoundTrackParameters::create(surface, geoCtx, pos4, dir, 1_e / p,
                                     std::nullopt, ParticleHypothesis::pion())
            .value();
    checkParameters(params, l0, l1, time, phi, theta, p, 1_e, pos, dir);
    BOOST_CHECK(!params.covariance());
  }
  // neutral any parameters from global information
  {
    auto params =
        BoundTrackParameters::create(surface, geoCtx, pos4, dir, 1 / p,
                                     std::nullopt, ParticleHypothesis::pion0())
            .value();
    checkParameters(params, l0, l1, time, phi, theta, p, 0_e, pos, dir);
    BOOST_CHECK(!params.covariance());
  }
  // double-negative any parameters from global information
  {
    auto params = BoundTrackParameters::create(
                      surface, geoCtx, pos4, dir, -2_e / p, std::nullopt,
                      ParticleHypothesis::pionLike(2_e))
                      .value();
    checkParameters(params, l0, l1, time, phi, theta, p, -2_e, pos, dir);
    BOOST_CHECK(!params.covariance());
  }
  // triple-positive any parameters from global information
  {
    auto params = BoundTrackParameters::create(
                      surface, geoCtx, pos4, dir, 3_e / p, std::nullopt,
                      ParticleHypothesis::pionLike(3_e))
                      .value();
    checkParameters(params, l0, l1, time, phi, theta, p, 3_e, pos, dir);
    BOOST_CHECK(!params.covariance());
  }
}

// different surfaces
// parameters must be chosen such that all possible local positions (as defined
// in the dataset's header) represent valid points on the surface.
const auto cones = bdata::make({
    Surface::makeShared<ConeSurface>(Transform3::Identity(),
                                     0.5 /* opening angle */),
});
const auto cylinders = bdata::make({
    Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                         10.0 /* radius */, 100 /* half z */),
});
const auto discs = bdata::make({
    Surface::makeShared<DiscSurface>(Transform3::Identity(), 0 /* radius min */,
                                     100 /* radius max */),
});
const auto perigees = bdata::make({
    Surface::makeShared<PerigeeSurface>(Vector3(0, 0, -1.5)),
});
const auto planes = bdata::make({
    CurvilinearSurface(Vector3(1, 2, 3), Vector3::UnitX()).planeSurface(),
    CurvilinearSurface(Vector3(-2, -3, -4), Vector3::UnitY()).planeSurface(),
    CurvilinearSurface(Vector3(3, -4, 5), Vector3::UnitZ()).planeSurface(),
});
const auto straws = bdata::make({
    Surface::makeShared<StrawSurface>(Transform3::Identity(), 2.0 /* radius */,
                                      200.0 /* half z */),
});

}  // namespace

BOOST_AUTO_TEST_SUITE(EventDataBoundTrackParameters)

BOOST_DATA_TEST_CASE(ConeSurface,
                     cones* posAngle* posPositiveNonzero* ts* phis* thetas* ps,
                     surface, lphi, lz, time, phi, theta, p) {
  // TODO extend lz to zero after fixing the transform implementation
  // local parameter r*phi has limits that depend on the z position
  const auto r = lz * surface->bounds().tanAlpha();
  // local coordinates are singular at z = 0 -> normalize local r*phi
  runTest(surface, (0 < lz) ? (r * lphi) : 0.0, lz, time, phi, theta, p);
}

BOOST_DATA_TEST_CASE(
    CylinderSurface,
    cylinders* posSymmetric* posSymmetric* ts* phis* thetas* ps, surface, lrphi,
    lz, time, phi, theta, p) {
  runTest(surface, lrphi, lz, time, phi, theta, p);
}

BOOST_DATA_TEST_CASE(DiscSurface,
                     discs* posPositive* posAngle* ts* phis* thetas* ps,
                     surface, lr, lphi, time, phi, theta, p) {
  // local coordinates are singular at r = 0 -> normalize local phi
  runTest(surface, lr, (0 < lr) ? lphi : 0.0, time, phi, theta, p);
}

BOOST_DATA_TEST_CASE(
    PerigeeSurface,
    perigees* posSymmetric* posSymmetric* ts* phis* thetasNoForwardBackward* ps,
    surface, d0, z0, time, phi, theta, p) {
  // TODO extend theta to forward/back extreme cases fixing the transform
  runTest(surface, d0, z0, time, phi, theta, p);
}

BOOST_DATA_TEST_CASE(PlaneSurface,
                     planes* posSymmetric* posSymmetric* ts* phis* thetas* ps,
                     surface, l0, l1, time, phi, theta, p) {
  runTest(surface, l0, l1, time, phi, theta, p);
}

BOOST_DATA_TEST_CASE(
    StrawSurface,
    straws* posPositive* posSymmetric* ts* phis* thetasNoForwardBackward* ps,
    surface, lr, lz, time, phi, theta, p) {
  // TODO extend theta to forward/back extreme cases fixing the transform
  runTest(surface, lr, lz, time, phi, theta, p);
}

BOOST_AUTO_TEST_SUITE_END()
