// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/Charge.hpp"
#include "Acts/EventData/GenericCurvilinearTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <cmath>
#include <limits>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

#include "TrackParametersDatasets.hpp"

namespace {

using namespace Acts;
using namespace Acts::UnitLiterals;

constexpr auto eps = 8 * std::numeric_limits<double>::epsilon();
const GeometryContext geoCtx;
const BoundSquareMatrix cov = BoundSquareMatrix::Identity();

void checkParameters(const CurvilinearTrackParameters& params, double phi,
                     double theta, double p, double q, const Vector4& pos4,
                     const Vector3& unitDir) {
  const auto qOverP = (q != 0) ? (q / p) : (1 / p);
  const auto pos = pos4.segment<3>(ePos0);

  const auto* referenceSurface =
      dynamic_cast<const PlaneSurface*>(&params.referenceSurface());
  BOOST_REQUIRE_MESSAGE(referenceSurface != nullptr,
                        "Reference surface is not a plane");

  // native values
  CHECK_SMALL(params.template get<eBoundLoc0>(), eps);
  CHECK_SMALL(params.template get<eBoundLoc1>(), eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eBoundTime>(), pos4[eTime], eps,
                       eps);
  CHECK_CLOSE_OR_SMALL(detail::radian_sym(params.template get<eBoundPhi>()),
                       detail::radian_sym(phi), eps, eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eBoundTheta>(), theta, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eBoundQOverP>(), qOverP, eps, eps);
  // convenience accessors
  CHECK_CLOSE_OR_SMALL(params.fourPosition(geoCtx), pos4, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.position(geoCtx), pos, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.time(), pos4[eTime], eps, eps);
  CHECK_CLOSE_OR_SMALL(params.direction(), unitDir, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.absoluteMomentum(), p, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.transverseMomentum(), p * std::sin(theta), eps,
                       eps);
  CHECK_CLOSE_OR_SMALL(params.momentum(), p * unitDir, eps, eps);
  BOOST_CHECK_EQUAL(params.charge(), q);
  // curvilinear reference surface
  CHECK_CLOSE_OR_SMALL(referenceSurface->center(geoCtx), pos, eps, eps);
  CHECK_CLOSE_OR_SMALL(referenceSurface->normal(geoCtx), unitDir, eps, eps);

  // reflection
  CurvilinearTrackParameters reflectedParams = params;
  reflectedParams.reflectInPlace();
  CHECK_CLOSE_OR_SMALL(params.reflect().parameters(),
                       reflectedParams.parameters(), eps, eps);
  CHECK_CLOSE_OR_SMALL(reflectedParams.reflect().parameters(),
                       params.parameters(), eps, eps);

  // TODO verify reference frame
}

}  // namespace

BOOST_AUTO_TEST_SUITE(EventDataCurvilinearTrackParameters)

BOOST_DATA_TEST_CASE(
    NeutralConstruct,
    posSymmetric* posSymmetric* posSymmetric* ts* phis* thetas* ps, x, y, z,
    time, phiInput, theta, p) {
  // phi is ill-defined in forward/backward tracks
  const auto phi = ((0 < theta) && (theta < std::numbers::pi)) ? phiInput : 0.;
  const Vector4 pos4(x, y, z, time);
  const Vector3 dir = makeDirectionFromPhiTheta(phi, theta);

  CurvilinearTrackParameters params(pos4, dir, 1 / p, std::nullopt,
                                    ParticleHypothesis::pion0());
  checkParameters(params, phi, theta, p, 0_e, pos4, dir);
  BOOST_CHECK(!params.covariance());

  // reassign w/ covariance
  params = CurvilinearTrackParameters(pos4, dir, 1 / p, cov,
                                      ParticleHypothesis::pion0());
  BOOST_CHECK(params.covariance());
  BOOST_CHECK_EQUAL(params.covariance().value(), cov);
}

BOOST_DATA_TEST_CASE(
    ChargedConstruct,
    posSymmetric* posSymmetric* posSymmetric* ts* phis* thetas* ps* qsNonZero,
    x, y, z, time, phiInput, theta, p, q) {
  // phi is ill-defined in forward/backward tracks
  const auto phi = ((0 < theta) && (theta < std::numbers::pi)) ? phiInput : 0.;
  const Vector4 pos4(x, y, z, time);
  const Vector3 dir = makeDirectionFromPhiTheta(phi, theta);

  CurvilinearTrackParameters params(pos4, dir, q / p, std::nullopt,
                                    ParticleHypothesis::pionLike(std::abs(q)));
  checkParameters(params, phi, theta, p, q, pos4, dir);
  BOOST_CHECK(!params.covariance());

  // reassign w/ covariance
  params = CurvilinearTrackParameters(
      pos4, dir, q / p, cov, ParticleHypothesis::pionLike(std::abs(q)));
  BOOST_CHECK(params.covariance());
  BOOST_CHECK_EQUAL(params.covariance().value(), cov);
}

BOOST_DATA_TEST_CASE(
    AnyConstruct,
    posSymmetric* posSymmetric* posSymmetric* ts* phis* thetas* ps* qsAny, x, y,
    z, time, phiInput, theta, p, q) {
  // phi is ill-defined in forward/backward tracks
  const auto phi = ((0 < theta) && (theta < std::numbers::pi)) ? phiInput : 0.;
  const Vector4 pos4(x, y, z, time);
  const Vector3 dir = makeDirectionFromPhiTheta(phi, theta);

  auto particleHypothesis = ParticleHypothesis::pionLike(std::abs(q));
  auto qOverP = particleHypothesis.qOverP(p, q);

  CurvilinearTrackParameters params(pos4, dir, qOverP, std::nullopt,
                                    particleHypothesis);
  checkParameters(params, phi, theta, p, q, pos4, dir);
  BOOST_CHECK(!params.covariance());

  // reassign w/ covariance
  params =
      CurvilinearTrackParameters(pos4, dir, qOverP, cov, particleHypothesis);
  BOOST_CHECK(params.covariance());
  BOOST_CHECK_EQUAL(params.covariance().value(), cov);
}

BOOST_AUTO_TEST_SUITE_END()
