// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/RiddersPropagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"

#include <limits>

#include "PropagationDatasets.hpp"
#include "PropagationTests.hpp"

namespace {

namespace ds = ActsTests::PropagationDatasets;
using namespace Acts::UnitLiterals;

using Stepper = Acts::StraightLineStepper;
using Propagator = Acts::Propagator<Stepper>;
using RiddersPropagator = Acts::RiddersPropagator<Propagator>;

// absolute parameter tolerances for position, direction, and absolute momentum
constexpr auto epsPos = 1_um;
constexpr auto epsDir = 0.125_mrad;
constexpr auto epsMom = 1_eV;
// relative covariance tolerance
constexpr auto epsCov = 0.0125;

const Acts::GeometryContext geoCtx;
const Acts::MagneticFieldContext magCtx;
const Stepper stepper;
const Propagator propagator(stepper);
const RiddersPropagator riddersPropagator(stepper);

}  // namespace

BOOST_AUTO_TEST_SUITE(PropagationStraightLine)

// check that the propagation is reversible and self-consistent

BOOST_DATA_TEST_CASE(
    ForwardBackward,
    ds::phi* ds::theta* ds::absMomentum* ds::chargeNonZero* ds::pathLength, phi,
    theta, p, q, s) {
  runForwardBackwardTest(propagator, geoCtx, magCtx,
                         makeParametersCurvilinear(phi, theta, p, q), s, epsPos,
                         epsDir, epsMom);
}

// check that reachable surfaces are correctly reached

// True forward/backward tracks do not work with z cylinders
BOOST_DATA_TEST_CASE(ToCylinderAlongZ,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength,
                     phi, theta, p, q, s) {
  runToSurfaceTest(propagator, geoCtx, magCtx,
                   makeParametersCurvilinear(phi, theta, p, q), s,
                   ZCylinderSurfaceBuilder(), epsPos, epsDir, epsMom);
}

BOOST_DATA_TEST_CASE(
    ToDisc,
    ds::phi* ds::theta* ds::absMomentum* ds::chargeNonZero* ds::pathLength, phi,
    theta, p, q, s) {
  runToSurfaceTest(propagator, geoCtx, magCtx,
                   makeParametersCurvilinear(phi, theta, p, q), s,
                   DiscSurfaceBuilder(), epsPos, epsDir, epsMom);
}

BOOST_DATA_TEST_CASE(
    ToPlane,
    ds::phi* ds::theta* ds::absMomentum* ds::chargeNonZero* ds::pathLength, phi,
    theta, p, q, s) {
  runToSurfaceTest(propagator, geoCtx, magCtx,
                   makeParametersCurvilinear(phi, theta, p, q), s,
                   PlaneSurfaceBuilder(), epsPos, epsDir, epsMom);
}

// True forward/backward tracks do not work with z straws
BOOST_DATA_TEST_CASE(ToStrawAlongZ,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength,
                     phi, theta, p, q, s) {
  runToSurfaceTest(propagator, geoCtx, magCtx,
                   makeParametersCurvilinear(phi, theta, p, q), s,
                   ZStrawSurfaceBuilder(), epsPos, epsDir, epsMom);
}

// check covariance transport using the ridders propagator for comparison

BOOST_DATA_TEST_CASE(CovarianceCurvilinear,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength,
                     phi, theta, p, q, s) {
  runForwardComparisonTest(
      propagator, riddersPropagator, geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s, epsPos,
      epsDir, epsMom, epsCov);
}

BOOST_DATA_TEST_CASE(CovarianceToCylinderAlongZ,
                     ds::phiWithoutAmbiguity* ds::thetaWithoutBeam*
                         ds::absMomentum* ds::chargeNonZero* ds::pathLength,
                     phi, theta, p, q, s) {
  runToSurfaceComparisonTest(
      propagator, riddersPropagator, geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      ZCylinderSurfaceBuilder(), epsPos, epsDir, epsMom, epsCov);
}

BOOST_DATA_TEST_CASE(CovarianceToDisc,
                     ds::phiWithoutAmbiguity* ds::thetaWithoutBeam*
                         ds::absMomentum* ds::chargeNonZero* ds::pathLength,
                     phi, theta, p, q, s) {
  runToSurfaceComparisonTest(
      propagator, riddersPropagator, geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      DiscSurfaceBuilder(), epsPos, epsDir, epsMom, epsCov);
}

BOOST_DATA_TEST_CASE(CovarianceToPlane,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength,
                     phi, theta, p, q, s) {
  runToSurfaceComparisonTest(
      propagator, riddersPropagator, geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      PlaneSurfaceBuilder(), epsPos, epsDir, epsMom, epsCov);
}

BOOST_DATA_TEST_CASE(CovarianceToStrawAlongZ,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength,
                     phi, theta, p, q, s) {
  // the numerical covariance transport to straw surfaces does not seem to be
  // stable. use a higher tolerance for now.
  runToSurfaceComparisonTest(
      propagator, riddersPropagator, geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      ZStrawSurfaceBuilder(), epsPos, epsDir, epsMom, 0.125);
}

BOOST_AUTO_TEST_SUITE_END()
