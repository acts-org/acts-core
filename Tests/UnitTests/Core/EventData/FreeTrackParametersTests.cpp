// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/Charge.hpp"
#include "Acts/EventData/GenericFreeTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/UnitVectors.hpp"

#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "TrackParametersDatasets.hpp"

namespace {

using namespace Acts;
using namespace Acts::UnitLiterals;

constexpr auto eps = 8 * std::numeric_limits<double>::epsilon();
const FreeSquareMatrix cov = FreeSquareMatrix::Identity();

void checkParameters(const FreeTrackParameters& params, const Vector4& pos4,
                     const Vector3& unitDir, double p, double q) {
  const auto qOverP = (q != 0) ? (q / p) : (1 / p);
  const auto pos = pos4.segment<3>(ePos0);

  // native values
  CHECK_CLOSE_OR_SMALL(params.template get<eFreePos0>(), pos4[ePos0], eps, eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eFreePos1>(), pos4[ePos1], eps, eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eFreePos2>(), pos4[ePos2], eps, eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eFreeTime>(), pos4[eTime], eps, eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eFreeDir0>(), unitDir[eMom0], eps,
                       eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eFreeDir1>(), unitDir[eMom1], eps,
                       eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eFreeDir2>(), unitDir[eMom2], eps,
                       eps);
  CHECK_CLOSE_OR_SMALL(params.template get<eFreeQOverP>(), qOverP, eps, eps);
  // convenience accessors
  CHECK_CLOSE_OR_SMALL(params.fourPosition(), pos4, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.position(), pos, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.time(), pos4[eFreeTime], eps, eps);
  CHECK_CLOSE_OR_SMALL(params.direction(), unitDir, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.absoluteMomentum(), p, eps, eps);
  CHECK_CLOSE_OR_SMALL(params.transverseMomentum(),
                       p * unitDir.template head<2>().norm(), eps, eps);
  CHECK_CLOSE_OR_SMALL(params.momentum(), p * unitDir, eps, eps);
  BOOST_CHECK_EQUAL(params.charge(), q);
  // self-consistency
  CHECK_CLOSE_OR_SMALL(params.position(),
                       params.parameters().template segment<3>(eFreePos0), eps,
                       eps);
  CHECK_CLOSE_OR_SMALL(params.time(), params.template get<eFreeTime>(), eps,
                       eps);

  // reflection
  FreeTrackParameters reflectedParams = params;
  reflectedParams.reflectInPlace();
  CHECK_CLOSE_OR_SMALL(params.reflect().parameters(),
                       reflectedParams.parameters(), eps, eps);
  CHECK_CLOSE_OR_SMALL(reflectedParams.reflect().parameters(),
                       params.parameters(), eps, eps);
}

}  // namespace

BOOST_AUTO_TEST_SUITE(CurvilinearTrackParameters)

BOOST_DATA_TEST_CASE(
    NeutralConstructFromAngles,
    posSymmetric* posSymmetric* posSymmetric* ts* phis* thetas* ps, x, y, z,
    time, phi, theta, p) {
  Vector4 pos4(x, y, z, time);
  Vector3 dir = makeDirectionFromPhiTheta(phi, theta);

  FreeTrackParameters params(pos4, phi, theta, 1 / p, std::nullopt,
                             ParticleHypothesis::pion0());
  checkParameters(params, pos4, dir, p, 0_e);
  BOOST_CHECK(!params.covariance());

  // reassign w/ covariance
  params = FreeTrackParameters(pos4, phi, theta, 1 / p, cov,
                               ParticleHypothesis::pion0());
  BOOST_CHECK(params.covariance());
  BOOST_CHECK_EQUAL(params.covariance().value(), cov);
}

BOOST_DATA_TEST_CASE(
    ChargedConstructFromAngles,
    posSymmetric* posSymmetric* posSymmetric* ts* phis* thetas* ps* qsNonZero,
    x, y, z, time, phi, theta, p, q) {
  Vector4 pos4(x, y, z, time);
  Vector3 dir = makeDirectionFromPhiTheta(phi, theta);

  FreeTrackParameters params(pos4, phi, theta, q / p, std::nullopt,
                             ParticleHypothesis::pionLike(std::abs(q)));
  checkParameters(params, pos4, dir, p, q);
  BOOST_CHECK(!params.covariance());

  // reassign w/ covariance
  params = FreeTrackParameters(pos4, phi, theta, q / p, cov,
                               ParticleHypothesis::pionLike(std::abs(q)));
  BOOST_CHECK(params.covariance());
  BOOST_CHECK_EQUAL(params.covariance().value(), cov);
}

BOOST_DATA_TEST_CASE(
    AnyConstructFromAngles,
    posSymmetric* posSymmetric* posSymmetric* ts* phis* thetas* ps* qsNonZero,
    x, y, z, time, phi, theta, p, q) {
  Vector4 pos4(x, y, z, time);
  Vector3 dir = makeDirectionFromPhiTheta(phi, theta);

  FreeTrackParameters params(pos4, phi, theta, q / p, std::nullopt,
                             ParticleHypothesis::pionLike(std::abs(q)));
  checkParameters(params, pos4, dir, p, q);
  BOOST_CHECK(!params.covariance());

  // reassign w/ covariance
  params = FreeTrackParameters(pos4, phi, theta, q / p, cov,
                               ParticleHypothesis::pionLike(std::abs(q)));
  BOOST_CHECK(params.covariance());
  BOOST_CHECK_EQUAL(params.covariance().value(), cov);
}

BOOST_AUTO_TEST_SUITE_END()
