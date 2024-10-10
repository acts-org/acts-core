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
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"

#include <limits>

#include "PropagationDatasets.hpp"
#include "PropagationTests.hpp"

namespace {

namespace ds = ActsTests::PropagationDatasets;
using namespace Acts::UnitLiterals;

using MagneticField = Acts::ConstantBField;
using EigenStepper = Acts::EigenStepper<>;
using EigenPropagator = Acts::Propagator<EigenStepper>;
using StraightLineStepper = Acts::StraightLineStepper;
using StraightLinePropagator = Acts::Propagator<StraightLineStepper>;

// absolute parameter tolerances for position, direction, and absolute momentum
constexpr auto epsPos = 1_um;
constexpr auto epsDir = 0.125_mrad;
constexpr auto epsMom = 1_eV;
// relative covariance tolerance
constexpr auto epsCov = 0.00125;

constexpr auto bz = 2_T;

const Acts::GeometryContext geoCtx;
const Acts::MagneticFieldContext magCtx;
const auto magFieldZero =
    std::make_shared<MagneticField>(Acts::Vector3::Zero());
const auto magFieldNonZero =
    std::make_shared<MagneticField>(Acts::Vector3::UnitZ() * bz);
const EigenPropagator eigenPropagatorZero{EigenStepper(magFieldZero)};
const EigenPropagator eigenPropagatorNonZero{EigenStepper(magFieldNonZero)};
const StraightLinePropagator straightPropagator{StraightLineStepper()};

}  // namespace

BOOST_AUTO_TEST_SUITE(PropagationCompareEigenStraightLine)

// TODO both the eigen stepper and the straight line stepper do not seem to
//      handle the neutral parameters correctly. the results contain inf/nan.
//      fix the propagators and re-enable the tests.

// BOOST_DATA_TEST_CASE(NeutralZeroMagneticField,
//                      ds::phi* ds::theta* ds::absMomentum* ds::pathLength,
//                      phi, theta, p, s) {
//   runFreePropagationComparisonTest(
//       eigenPropagatorZero, straightPropagator, geoCtx, magCtx,
//       makeParametersCurvilinearNeutral(phi, theta, p), s, epsPos, epsDir,
//       epsMom, epsCov);
// }

// BOOST_DATA_TEST_CASE(NeutralNonZeroMagneticField,
//                      ds::phi* ds::theta* ds::absMomentum* ds::pathLength,
//                      phi, theta, p, s) {
//   runFreePropagationComparisonTest(
//       eigenPropagatorNonZero, straightPropagator, geoCtx, magCtx,
//       makeParametersCurvilinearNeutral(phi, theta, p), s, epsPos, epsDir,
//       epsMom, epsCov);
// }

BOOST_DATA_TEST_CASE(
    ChargedZeroMagneticField,
    ds::phi* ds::theta* ds::absMomentum* ds::chargeNonZero* ds::pathLength, phi,
    theta, p, q, s) {
  runForwardComparisonTest(eigenPropagatorZero, straightPropagator, geoCtx,
                           magCtx, makeParametersCurvilinear(phi, theta, p, q),
                           s, epsPos, epsDir, epsMom, epsCov);
}

// TODO add comparison tests between the straight line and eigen propagator for
//      a charged particle w/ infinite momentum in a non-zero magnetic field.
//      these should be identical. requires proper handling of q/p=0 in the
//      track parameters.

BOOST_AUTO_TEST_SUITE_END()
