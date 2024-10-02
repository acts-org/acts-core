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
#include "Acts/Propagator/AtlasStepper.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"

#include <limits>
#include <utility>

#include "PropagationDatasets.hpp"
#include "PropagationTests.hpp"

namespace {

namespace ds = ActsTests::PropagationDatasets;
using namespace Acts::UnitLiterals;

using MagneticField = Acts::ConstantBField;
using AtlasStepper = Acts::AtlasStepper;
using AtlasPropagator = Acts::Propagator<AtlasStepper>;
using EigenStepper = Acts::EigenStepper<>;
using EigenPropagator = Acts::Propagator<EigenStepper>;

// absolute parameter tolerances for position, direction, and absolute momentum
constexpr auto epsPos = 1_um;
constexpr auto epsDir = 0.125_mrad;
constexpr auto epsMom = 1_eV;
// relative covariance tolerance
constexpr auto epsCov = 0.1;

const Acts::GeometryContext geoCtx;
const Acts::MagneticFieldContext magCtx;

inline std::pair<AtlasPropagator, EigenPropagator> makePropagators(double bz) {
  auto field = std::make_shared<MagneticField>(Acts::Vector3(0.0, 0.0, bz));
  return {AtlasPropagator(AtlasStepper(field)),
          EigenPropagator(EigenStepper(field))};
}

}  // namespace

BOOST_AUTO_TEST_SUITE(PropagationCompareAtlasEigenConstant)

BOOST_DATA_TEST_CASE(Forward,
                     ds::phi*(ds::thetaWithoutBeam)*ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  auto [atlasPropagator, eigenPropagator] = makePropagators(bz);
  runForwardComparisonTest(
      atlasPropagator, eigenPropagator, geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s, epsPos,
      epsDir, epsMom, epsCov);
}

BOOST_DATA_TEST_CASE(ToCylinderAlongZ,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  auto [atlasPropagator, eigenPropagator] = makePropagators(bz);
  runToSurfaceComparisonTest(
      atlasPropagator, eigenPropagator, geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      ZCylinderSurfaceBuilder(), epsPos, epsDir, epsMom, epsCov);
}

BOOST_DATA_TEST_CASE(
    ToDisc,
    ds::phiWithoutAmbiguity* ds::thetaWithoutBeam* ds::absMomentum*
        ds::chargeNonZero* ds::pathLength* ds::magneticField,
    phi, theta, p, q, s, bz) {
  auto [atlasPropagator, eigenPropagator] = makePropagators(bz);
  runToSurfaceComparisonTest(
      atlasPropagator, eigenPropagator, geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      DiscSurfaceBuilder(), epsPos, epsDir, epsMom, epsCov);
}

BOOST_DATA_TEST_CASE(ToPlane,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  auto [atlasPropagator, eigenPropagator] = makePropagators(bz);
  runToSurfaceComparisonTest(
      atlasPropagator, eigenPropagator, geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      PlaneSurfaceBuilder(), epsPos, epsDir, epsMom, epsCov);
}

BOOST_DATA_TEST_CASE(ToStrawAlongZ,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  auto [atlasPropagator, eigenPropagator] = makePropagators(bz);
  runToSurfaceComparisonTest(
      atlasPropagator, eigenPropagator, geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      ZStrawSurfaceBuilder(), epsPos, epsDir, epsMom, epsCov);
}

BOOST_AUTO_TEST_SUITE_END()
