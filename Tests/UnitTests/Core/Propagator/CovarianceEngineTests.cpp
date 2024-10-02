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
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/TransformationHelpers.hpp"
#include "Acts/EventData/detail/CorrectedTransformationFreeToBound.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/VoidNavigator.hpp"
#include "Acts/Propagator/detail/CovarianceEngine.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cmath>
#include <memory>
#include <optional>
#include <random>
#include <tuple>
#include <utility>

namespace bdata = boost::unit_test::data;

namespace Acts::Test {

Acts::GeometryContext gctx;
Acts::MagneticFieldContext mctx;

using namespace Acts::UnitLiterals;

using Covariance = BoundSquareMatrix;
using Jacobian = BoundMatrix;

/// These tests do not test for a correct covariance transport but only for the
/// correct conservation or modification of certain variables. A test suite for
/// the numerical correctness is performed in the integration tests.
BOOST_AUTO_TEST_CASE(covariance_engine_test) {
  // Create a test context
  GeometryContext tgContext = GeometryContext();

  auto particleHypothesis = ParticleHypothesis::pion();

  // Build a start vector
  Vector3 position{1., 2., 3.};
  double time = 4.;
  Vector3 direction{sqrt(5. / 22.), 3. * sqrt(2. / 55.), 7. / sqrt(110.)};
  double qop = 0.125;
  FreeVector parameters, startParameters;
  parameters << position[0], position[1], position[2], time, direction[0],
      direction[1], direction[2], qop;
  startParameters = parameters;

  // Build covariance matrix, jacobians and related components
  Covariance covariance = Covariance::Identity();
  Jacobian jacobian = 2. * Jacobian::Identity();
  FreeMatrix transportJacobian = 3. * FreeMatrix::Identity();
  FreeVector derivatives;
  derivatives << 9., 10., 11., 12., 13., 14., 15., 16.;
  BoundToFreeMatrix boundToFreeJacobian = 4. * BoundToFreeMatrix::Identity();

  // Covariance transport to curvilinear coordinates
  detail::transportCovarianceToCurvilinear(covariance, jacobian,
                                           transportJacobian, derivatives,
                                           boundToFreeJacobian, direction);

  // Tests to see that the right components are (un-)changed
  BOOST_CHECK_NE(covariance, Covariance::Identity());
  BOOST_CHECK_NE(jacobian, 2. * Jacobian::Identity());
  BOOST_CHECK_EQUAL(transportJacobian, FreeMatrix::Identity());
  BOOST_CHECK_EQUAL(derivatives, FreeVector::Zero());
  BOOST_CHECK_NE(boundToFreeJacobian, 4. * BoundToFreeMatrix::Identity());
  BOOST_CHECK_EQUAL(
      direction, Vector3(sqrt(5. / 22.), 3. * sqrt(2. / 55.), 7. / sqrt(110.)));

  // Reset
  covariance = Covariance::Identity();
  jacobian = 2. * Jacobian::Identity();
  transportJacobian = 3. * FreeMatrix::Identity();
  derivatives << 9., 10., 11., 12., 13., 14., 15., 16.;
  boundToFreeJacobian = 4. * BoundToFreeMatrix::Identity();

  // Repeat transport to surface
  FreeToBoundCorrection freeToBoundCorrection(false);
  auto surface = CurvilinearSurface(position, direction).planeSurface();
  detail::transportCovarianceToBound(
      tgContext, *surface, covariance, jacobian, transportJacobian, derivatives,
      boundToFreeJacobian, parameters, freeToBoundCorrection);

  BOOST_CHECK_NE(covariance, Covariance::Identity());
  BOOST_CHECK_NE(jacobian, 2. * Jacobian::Identity());
  BOOST_CHECK_EQUAL(transportJacobian, FreeMatrix::Identity());
  BOOST_CHECK_EQUAL(derivatives, FreeVector::Zero());
  BOOST_CHECK_NE(boundToFreeJacobian, 4. * BoundToFreeMatrix::Identity());
  BOOST_CHECK_EQUAL(parameters, startParameters);

  // Produce a curvilinear state without covariance matrix
  auto curvResult = detail::curvilinearState(
      covariance, jacobian, transportJacobian, derivatives, boundToFreeJacobian,
      parameters, particleHypothesis, false, 1337.);
  BOOST_CHECK(!std::get<0>(curvResult).covariance().has_value());
  BOOST_CHECK_EQUAL(std::get<2>(curvResult), 1337.);

  // Reset
  covariance = Covariance::Identity();
  jacobian = 2. * Jacobian::Identity();
  transportJacobian = 3. * FreeMatrix::Identity();
  derivatives << 9., 10., 11., 12., 13., 14., 15., 16.;
  boundToFreeJacobian = 4. * BoundToFreeMatrix::Identity();

  // Produce a curvilinear state with covariance matrix
  curvResult = detail::curvilinearState(
      covariance, jacobian, transportJacobian, derivatives, boundToFreeJacobian,
      parameters, particleHypothesis, true, 1337.);
  BOOST_CHECK(std::get<0>(curvResult).covariance().has_value());
  BOOST_CHECK_NE(*(std::get<0>(curvResult).covariance()),
                 Covariance::Identity());
  BOOST_CHECK_NE(std::get<1>(curvResult), 2. * Jacobian::Identity());
  BOOST_CHECK_EQUAL(std::get<2>(curvResult), 1337.);

  // Produce a bound state without covariance matrix
  auto covarianceBefore = covariance;
  auto boundResult =
      detail::boundState(tgContext, *surface, covariance, jacobian,
                         transportJacobian, derivatives, boundToFreeJacobian,
                         parameters, particleHypothesis, false, 1337.,
                         freeToBoundCorrection)
          .value();
  BOOST_CHECK(std::get<0>(curvResult).covariance().has_value());
  BOOST_CHECK_EQUAL(*(std::get<0>(curvResult).covariance()), covarianceBefore);
  BOOST_CHECK_EQUAL(std::get<2>(boundResult), 1337.);

  // Reset
  covariance = Covariance::Identity();
  jacobian = 2. * Jacobian::Identity();
  transportJacobian = 3. * FreeMatrix::Identity();
  derivatives << 9., 10., 11., 12., 13., 14., 15., 16.;
  boundToFreeJacobian = 4. * BoundToFreeMatrix::Identity();

  // Produce a bound state with covariance matrix
  boundResult =
      detail::boundState(tgContext, *surface, covariance, jacobian,
                         transportJacobian, derivatives, boundToFreeJacobian,
                         parameters, ParticleHypothesis::pion(), true, 1337.,
                         freeToBoundCorrection)
          .value();
  BOOST_CHECK(std::get<0>(boundResult).covariance().has_value());
  BOOST_CHECK_NE(*(std::get<0>(boundResult).covariance()),
                 Covariance::Identity());
  BOOST_CHECK_NE(std::get<1>(boundResult), 2. * Jacobian::Identity());
  BOOST_CHECK_EQUAL(std::get<2>(boundResult), 1337.);

  // Reset
  freeToBoundCorrection.apply = true;

  // Produce a bound state with free to bound correction
  boundResult =
      detail::boundState(tgContext, *surface, covariance, jacobian,
                         transportJacobian, derivatives, boundToFreeJacobian,
                         parameters, ParticleHypothesis::pion(), true, 1337.,
                         freeToBoundCorrection)
          .value();
  BOOST_CHECK(std::get<0>(boundResult).covariance().has_value());
  BOOST_CHECK_NE(*(std::get<0>(boundResult).covariance()),
                 Covariance::Identity());
}

std::pair<BoundVector, BoundMatrix> boundToBound(const BoundVector& parIn,
                                                 const BoundMatrix& covIn,
                                                 const Surface& srfA,
                                                 const Surface& srfB,
                                                 const Vector3& bField) {
  Acts::BoundTrackParameters boundParamIn{srfA.getSharedPtr(), parIn, covIn,
                                          ParticleHypothesis::pion()};

  auto converted =
      detail::boundToBoundConversion(gctx, boundParamIn, srfB, bField).value();

  return {converted.parameters(), converted.covariance().value()};
}

using propagator_t = Propagator<EigenStepper<>, VoidNavigator>;

BoundVector localToLocal(const propagator_t& prop, const BoundVector& local,
                         const Surface& src, const Surface& dst) {
  using PropagatorOptions = typename propagator_t::template Options<>;
  PropagatorOptions options{gctx, mctx};
  options.stepping.stepTolerance = 1e-10;
  options.surfaceTolerance = 1e-10;

  BoundTrackParameters start{src.getSharedPtr(), local, std::nullopt,
                             ParticleHypothesis::pion()};

  auto res = prop.propagate(start, dst, options).value();
  auto endParameters = res.endParameters.value();

  BOOST_CHECK_EQUAL(&endParameters.referenceSurface(), &dst);

  BoundVector out = endParameters.parameters();
  out[eBoundTime] = local[eBoundTime];
  return out;
}

propagator_t makePropagator(const Vector3& bField) {
  return propagator_t{EigenStepper<>{std::make_shared<ConstantBField>(bField)},
                      VoidNavigator{}};
}

BoundMatrix numericalBoundToBoundJacobian(const propagator_t& prop,
                                          const BoundVector& parA,
                                          const Surface& srfA,
                                          const Surface& srfB) {
  double h = 1e-4;
  BoundMatrix J;
  for (std::size_t i = 0; i < 6; i++) {
    for (std::size_t j = 0; j < 6; j++) {
      BoundVector parInitial1 = parA;
      BoundVector parInitial2 = parA;
      parInitial1[i] -= h;
      parInitial2[i] += h;
      BoundVector parFinal1 = localToLocal(prop, parInitial1, srfA, srfB);
      BoundVector parFinal2 = localToLocal(prop, parInitial2, srfA, srfB);

      J(j, i) = (parFinal2[j] - parFinal1[j]) / (2 * h);
    }
  }
  return J;
}

unsigned int getNextSeed() {
  static unsigned int seed = 10;
  return ++seed;
}

auto makeDist(double a, double b) {
  return bdata::random(
      (bdata::engine = std::mt19937{}, bdata::seed = getNextSeed(),
       bdata::distribution = std::uniform_real_distribution<double>(a, b)));
}

const auto locDist = makeDist(-5_mm, 5_mm);
const auto bFieldDist = makeDist(0, 3_T);
const auto angleDist = makeDist(-2 * M_PI, 2 * M_PI);
const auto posDist = makeDist(-50_mm, 50_mm);

#define MAKE_SURFACE()                                    \
  [&]() {                                                 \
    Transform3 transformA = Transform3::Identity();       \
    transformA = AngleAxis3(Rx, Vector3::UnitX());        \
    transformA = AngleAxis3(Ry, Vector3::UnitY());        \
    transformA = AngleAxis3(Rz, Vector3::UnitZ());        \
    transformA.translation() << gx, gy, gz;               \
    return Surface::makeShared<PlaneSurface>(transformA); \
  }()

BOOST_DATA_TEST_CASE(CovarianceConversionSamePlane,
                     (bFieldDist ^ bFieldDist ^ bFieldDist ^ angleDist ^
                      angleDist ^ angleDist ^ posDist ^ posDist ^ posDist ^
                      locDist ^ locDist) ^
                         bdata::xrange(100),
                     Bx, By, Bz, Rx, Ry, Rz, gx, gy, gz, l0, l1, index) {
  (void)index;
  const Vector3 bField{Bx, By, Bz};

  auto planeSurfaceA = MAKE_SURFACE();
  auto planeSurfaceB =
      Surface::makeShared<PlaneSurface>(planeSurfaceA->transform(gctx));

  BoundMatrix covA;
  covA.setZero();
  covA.diagonal() << 1, 2, 3, 4, 5, 6;

  BoundVector parA;
  parA << l0, l1, M_PI / 4., M_PI_2 * 0.9, -1 / 1_GeV, 5_ns;

  // identical surface, this should work
  auto [parB, covB] =
      boundToBound(parA, covA, *planeSurfaceA, *planeSurfaceB, bField);

  // these should be the same because the plane surface are the same
  CHECK_CLOSE_ABS(parA, parB, 1e-9);
  CHECK_CLOSE_COVARIANCE(covA, covB, 1e-9);

  // now go back
  auto [parA2, covA2] =
      boundToBound(parB, covB, *planeSurfaceB, *planeSurfaceA, bField);
  CHECK_CLOSE_ABS(parA, parA2, 1e-9);
  CHECK_CLOSE_COVARIANCE(covA, covA2, 1e-9);

  auto prop = makePropagator(bField);

  BoundMatrix J =
      numericalBoundToBoundJacobian(prop, parA, *planeSurfaceA, *planeSurfaceB);
  BoundMatrix covC = J * covA * J.transpose();
  CHECK_CLOSE_COVARIANCE(covB, covC, 1e-6);
}

BOOST_DATA_TEST_CASE(CovarianceConversionRotatedPlane,
                     (bFieldDist ^ bFieldDist ^ bFieldDist ^ angleDist ^
                      angleDist ^ angleDist ^ posDist ^ posDist ^ posDist ^
                      locDist ^ locDist ^ angleDist) ^
                         bdata::xrange(100),
                     Bx, By, Bz, Rx, Ry, Rz, gx, gy, gz, l0, l1, angle, index) {
  (void)index;
  const Vector3 bField{Bx, By, Bz};

  auto planeSurfaceA = MAKE_SURFACE();

  Transform3 transform;
  transform = planeSurfaceA->transform(gctx).rotation();
  transform = AngleAxis3(angle, planeSurfaceA->normal(gctx)) * transform;
  transform.translation() = planeSurfaceA->transform(gctx).translation();
  auto planeSurfaceB = Surface::makeShared<PlaneSurface>(transform);

  // sanity check that the normal didn't change
  CHECK_CLOSE_ABS(planeSurfaceA->normal(gctx), planeSurfaceB->normal(gctx),
                  1e-9);

  BoundMatrix covA;
  covA.setZero();
  covA.diagonal() << 1, 2, 3, 4, 5, 6;

  BoundVector parA;
  parA << l0, l1, M_PI / 4., M_PI_2 * 0.9, -1 / 1_GeV, 5_ns;

  auto [parB, covB] =
      boundToBound(parA, covA, *planeSurfaceA, *planeSurfaceB, bField);
  BoundVector exp = parA;
  // loc0 and loc1 are rotated
  exp.head<2>() = Eigen::Rotation2D<double>(-angle) * parA.head<2>();

  CHECK_CLOSE_ABS(exp, parB, 1e-9);

  // now go back
  auto [parA2, covA2] =
      boundToBound(parB, covB, *planeSurfaceB, *planeSurfaceA, bField);
  CHECK_CLOSE_ABS(parA, parA2, 1e-9);
  CHECK_CLOSE_COVARIANCE(covA, covA2, 1e-9);

  auto prop = makePropagator(bField);
  BoundMatrix J =
      numericalBoundToBoundJacobian(prop, parA, *planeSurfaceA, *planeSurfaceB);
  BoundMatrix covC = J * covA * J.transpose();
  CHECK_CLOSE_COVARIANCE(covB, covC, 1e-6);
}

BOOST_DATA_TEST_CASE(CovarianceConversionL0TiltedPlane,
                     (bFieldDist ^ bFieldDist ^ bFieldDist ^ angleDist ^
                      angleDist ^ angleDist ^ posDist ^ posDist ^ posDist ^
                      locDist ^ angleDist) ^
                         bdata::xrange(100),
                     Bx, By, Bz, Rx, Ry, Rz, gx, gy, gz, l1, angle, index) {
  (void)index;
  const Vector3 bField{Bx, By, Bz};

  auto planeSurfaceA = MAKE_SURFACE();

  // make plane that is slightly rotated
  Transform3 transform;
  transform = planeSurfaceA->transform(gctx).rotation();

  // figure out rotation axis along local x
  Vector3 axis = planeSurfaceA->transform(gctx).rotation() * Vector3::UnitY();
  transform = AngleAxis3(angle, axis) * transform;

  transform.translation() = planeSurfaceA->transform(gctx).translation();

  auto planeSurfaceB = Surface::makeShared<PlaneSurface>(transform);

  BoundVector parA;
  // loc 0 must be zero so we're on the intersection of both surfaces.
  parA << 0, l1, M_PI / 4., M_PI_2 * 0.9, -1 / 1_GeV, 5_ns;

  BoundMatrix covA;
  covA.setZero();
  covA.diagonal() << 1, 2, 3, 4, 5, 6;

  auto [parB, covB] =
      boundToBound(parA, covA, *planeSurfaceA, *planeSurfaceB, bField);

  // now go back
  auto [parA2, covA2] =
      boundToBound(parB, covB, *planeSurfaceB, *planeSurfaceA, bField);
  CHECK_CLOSE_ABS(parA, parA2, 1e-9);
  CHECK_CLOSE_COVARIANCE(covA, covA2, 1e-7);

  auto prop = makePropagator(bField);
  BoundMatrix J =
      numericalBoundToBoundJacobian(prop, parA, *planeSurfaceA, *planeSurfaceB);
  BoundMatrix covC = J * covA * J.transpose();
  CHECK_CLOSE_OR_SMALL((covB.template topLeftCorner<2, 2>()),
                       (covC.template topLeftCorner<2, 2>()), 1e-7, 1e-9);
  CHECK_CLOSE_OR_SMALL(covB.diagonal(), covC.diagonal(), 1e-7, 1e-9);
}

BOOST_DATA_TEST_CASE(CovarianceConversionL1TiltedPlane,
                     (bFieldDist ^ bFieldDist ^ bFieldDist ^ angleDist ^
                      angleDist ^ angleDist ^ posDist ^ posDist ^ posDist ^
                      locDist ^ angleDist) ^
                         bdata::xrange(100),
                     Bx, By, Bz, Rx, Ry, Rz, gx, gy, gz, l0, angle, index) {
  (void)index;
  const Vector3 bField{Bx, By, Bz};

  auto planeSurfaceA = MAKE_SURFACE();

  // make plane that is slightly rotated
  Transform3 transform;
  transform = planeSurfaceA->transform(gctx).rotation();

  Vector3 axis = planeSurfaceA->transform(gctx).rotation() * Vector3::UnitX();
  transform = AngleAxis3(angle, axis) * transform;

  transform.translation() = planeSurfaceA->transform(gctx).translation();

  auto planeSurfaceB = Surface::makeShared<PlaneSurface>(transform);

  BoundVector parA;
  // loc 1 must be zero so we're on the intersection of both surfaces.
  parA << l0, 0, M_PI / 4., M_PI_2 * 0.9, -1 / 1_GeV, 5_ns;

  BoundMatrix covA;
  covA.setZero();
  covA.diagonal() << 1, 2, 3, 4, 5, 6;

  auto [parB, covB] =
      boundToBound(parA, covA, *planeSurfaceA, *planeSurfaceB, bField);

  // now go back
  auto [parA2, covA2] =
      boundToBound(parB, covB, *planeSurfaceB, *planeSurfaceA, bField);
  CHECK_CLOSE_ABS(parA, parA2, 1e-9);
  // tolerance is a bit higher here
  CHECK_CLOSE_COVARIANCE(covA, covA2, 1e-6);

  auto prop = makePropagator(bField);
  BoundMatrix J =
      numericalBoundToBoundJacobian(prop, parA, *planeSurfaceA, *planeSurfaceB);
  BoundMatrix covC = J * covA * J.transpose();
  CHECK_CLOSE_OR_SMALL((covB.template topLeftCorner<2, 2>()),
                       (covC.template topLeftCorner<2, 2>()), 1e-6, 1e-9);
  CHECK_CLOSE_OR_SMALL(covB.diagonal(), covC.diagonal(), 1e-6, 1e-9);
}

BOOST_DATA_TEST_CASE(CovarianceConversionPerigee,
                     (bFieldDist ^ bFieldDist ^ bFieldDist ^ angleDist ^
                      angleDist ^ angleDist ^ posDist ^ posDist ^ posDist ^
                      locDist ^ locDist ^ angleDist ^ angleDist ^ angleDist) ^
                         bdata::xrange(100),
                     Bx, By, Bz, Rx, Ry, Rz, gx, gy, gz, l0, l1, pRx, pRy, pRz,
                     index) {
  (void)index;
  const Vector3 bField{Bx, By, Bz};

  auto planeSurfaceA = MAKE_SURFACE();

  BoundVector parA;
  parA << l0, l1, M_PI / 4., M_PI_2 * 0.9, -1 / 1_GeV, 5_ns;

  BoundMatrix covA;
  covA.setZero();
  covA.diagonal() << 1, 2, 3, 4, 5, 6;

  Vector3 global = planeSurfaceA->localToGlobal(gctx, parA.head<2>());

  Transform3 transform;
  transform.setIdentity();
  transform.rotate(AngleAxis3(pRx, Vector3::UnitX()));
  transform.rotate(AngleAxis3(pRy, Vector3::UnitY()));
  transform.rotate(AngleAxis3(pRz, Vector3::UnitZ()));
  transform.translation() = global;

  auto perigee = Surface::makeShared<PerigeeSurface>(transform);

  auto [parB, covB] =
      boundToBound(parA, covA, *planeSurfaceA, *perigee, bField);

  // now go back
  auto [parA2, covA2] =
      boundToBound(parB, covB, *perigee, *planeSurfaceA, bField);
  CHECK_CLOSE_ABS(parA, parA2, 1e-9);
  CHECK_CLOSE_COVARIANCE(covA, covA2, 1e-9);

  auto prop = makePropagator(bField);
  BoundMatrix J =
      numericalBoundToBoundJacobian(prop, parA, *planeSurfaceA, *perigee);
  BoundMatrix covC = J * covA * J.transpose();
  CHECK_CLOSE_ABS(covB, covC, 1e-7);
}

}  // namespace Acts::Test
