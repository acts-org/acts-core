// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/CuboidVolumeBuilder.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingGeometryBuilder.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/HomogeneousVolumeMaterial.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/EigenStepperDenseExtension.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/RiddersPropagator.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <limits>

#include "PropagationDatasets.hpp"
#include "PropagationTests.hpp"

namespace {

namespace ds = ActsTests::PropagationDatasets;
using namespace Acts::UnitLiterals;

using MagneticField = Acts::ConstantBField;
using Stepper = Acts::EigenStepper<Acts::EigenStepperDenseExtension>;
using Propagator = Acts::Propagator<Stepper, Acts::Navigator>;
using RiddersPropagator = Acts::RiddersPropagator<Propagator>;

// absolute parameter tolerances for position, direction, and absolute momentum
constexpr auto epsPos = 10_um;
constexpr auto epsDir = 1_mrad;
constexpr auto epsMom = 5_MeV;
// relative covariance tolerance
constexpr auto epsCov = 0.07;

const Acts::GeometryContext geoCtx;
const Acts::MagneticFieldContext magCtx;

inline std::shared_ptr<const Acts::TrackingGeometry> makeDetector() {
  using namespace Acts;

  // avoid rebuilding the tracking geometry for every propagator
  static std::shared_ptr<const Acts::TrackingGeometry> detector;
  if (!detector) {
    CuboidVolumeBuilder::VolumeConfig vConf;
    vConf.position = {0., 0., 0.};
    vConf.length = {4_m, 4_m, 4_m};
    vConf.volumeMaterial = std::make_shared<const HomogeneousVolumeMaterial>(
        Acts::Test::makeBeryllium());
    CuboidVolumeBuilder::Config conf;
    conf.volumeCfg.push_back(vConf);
    conf.position = {0., 0., 0.};
    conf.length = {4_m, 4_m, 4_m};
    CuboidVolumeBuilder cvb(conf);
    TrackingGeometryBuilder::Config tgbCfg;
    tgbCfg.trackingVolumeBuilders.push_back(
        [=](const auto& context, const auto& inner, const auto&) {
          return cvb.trackingVolume(context, inner, nullptr);
        });
    detector = TrackingGeometryBuilder(tgbCfg).trackingGeometry(geoCtx);
  }

  return detector;
}

inline Propagator makePropagator(double bz) {
  using namespace Acts;

  auto magField = std::make_shared<MagneticField>(Acts::Vector3(0.0, 0.0, bz));
  Stepper stepper(std::move(magField));
  auto logger = getDefaultLogger("Nominal", Logging::INFO);
  return Propagator(
      std::move(stepper),
      Acts::Navigator{{makeDetector()}, logger->cloneWithSuffix("Nav")},
      logger->cloneWithSuffix("Prop"));
}

inline RiddersPropagator makeRiddersPropagator(double bz) {
  using namespace Acts;

  auto magField = std::make_shared<MagneticField>(Acts::Vector3(0.0, 0.0, bz));
  Stepper stepper(std::move(magField));
  auto logger = getDefaultLogger("Ridder", Logging::INFO);
  auto propagator = Propagator(
      std::move(stepper),
      Acts::Navigator{{makeDetector()}, logger->cloneWithSuffix("Nav")},
      logger->cloneWithSuffix("Prop"));
  return RiddersPropagator(std::move(propagator));
}

}  // namespace

BOOST_AUTO_TEST_SUITE(PropagationDenseConstant)

// check that the propagation is reversible and self-consistent

// TODO does not seem to work as-is
BOOST_DATA_TEST_CASE(ForwardBackward,
                     ds::phi* ds::theta* ds::absMomentum* ds::chargeNonZero*
                         ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  runForwardBackwardTest<Propagator>(
      makePropagator(bz), geoCtx, magCtx,
      makeParametersCurvilinear(phi, theta, p, q), s, epsPos, epsDir, epsMom);
}

// check that reachable surfaces are correctly reached

// True forward/backward tracks do not work with z cylinders
BOOST_DATA_TEST_CASE(ToCylinderAlongZ,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  runToSurfaceTest<Propagator, ZCylinderSurfaceBuilder>(
      makePropagator(bz), geoCtx, magCtx,
      makeParametersCurvilinear(phi, theta, p, q), s, ZCylinderSurfaceBuilder(),
      epsPos, epsDir, epsMom);
}

BOOST_DATA_TEST_CASE(ToDisc,
                     ds::phi* ds::theta* ds::absMomentum* ds::chargeNonZero*
                         ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  runToSurfaceTest<Propagator, DiscSurfaceBuilder>(
      makePropagator(bz), geoCtx, magCtx,
      makeParametersCurvilinear(phi, theta, p, q), s, DiscSurfaceBuilder(),
      epsPos, epsDir, epsMom);
}

BOOST_DATA_TEST_CASE(ToPlane,
                     ds::phi* ds::theta* ds::absMomentum* ds::chargeNonZero*
                         ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  runToSurfaceTest<Propagator, PlaneSurfaceBuilder>(
      makePropagator(bz), geoCtx, magCtx,
      makeParametersCurvilinear(phi, theta, p, q), s, PlaneSurfaceBuilder(),
      epsPos, epsDir, epsMom);
}

// True forward/backward tracks do not work with z straws
BOOST_DATA_TEST_CASE(ToStrawAlongZ,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  runToSurfaceTest<Propagator, ZStrawSurfaceBuilder>(
      makePropagator(bz), geoCtx, magCtx,
      makeParametersCurvilinear(phi, theta, p, q), s, ZStrawSurfaceBuilder(),
      epsPos, epsDir, epsMom);
}

// check covariance transport using the ridders propagator for comparison
// Covariance transport does not work for theta close to poles
BOOST_DATA_TEST_CASE(CovarianceCurvilinear,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  runForwardComparisonTest<Propagator, RiddersPropagator>(
      makePropagator(bz), makeRiddersPropagator(bz), geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s, epsPos,
      epsDir, epsMom, epsCov);
}

// limit theta to ignore the covariance mismatches at high theta for now
BOOST_DATA_TEST_CASE(CovarianceToCylinderAlongZ,
                     ds::phiWithoutAmbiguity* ds::thetaCentral* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  runToSurfaceComparisonTest<Propagator, RiddersPropagator,
                             ZCylinderSurfaceBuilder>(
      makePropagator(bz), makeRiddersPropagator(bz), geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      ZCylinderSurfaceBuilder(), epsPos, epsDir, epsMom, epsCov);
}

BOOST_DATA_TEST_CASE(CovarianceToDisc,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  runToSurfaceComparisonTest<Propagator, RiddersPropagator, DiscSurfaceBuilder>(
      makePropagator(bz), makeRiddersPropagator(bz), geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      DiscSurfaceBuilder(), epsPos, epsDir, epsMom, epsCov);
}

// Covariance transport does not work for theta close to poles
BOOST_DATA_TEST_CASE(CovarianceToPlane,
                     ds::phi* ds::thetaWithoutBeam* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  runToSurfaceComparisonTest<Propagator, RiddersPropagator,
                             PlaneSurfaceBuilder>(
      makePropagator(bz), makeRiddersPropagator(bz), geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      PlaneSurfaceBuilder(), epsPos, epsDir, epsMom, epsCov);
}

// limit theta to ignore the covariance mismatches at high theta for now
BOOST_DATA_TEST_CASE(CovarianceToStrawAlongZ,
                     ds::phiWithoutAmbiguity* ds::thetaCentral* ds::absMomentum*
                         ds::chargeNonZero* ds::pathLength* ds::magneticField,
                     phi, theta, p, q, s, bz) {
  runToSurfaceComparisonTest<Propagator, RiddersPropagator,
                             ZStrawSurfaceBuilder>(
      makePropagator(bz), makeRiddersPropagator(bz), geoCtx, magCtx,
      makeParametersCurvilinearWithCovariance(phi, theta, p, q), s,
      ZStrawSurfaceBuilder(), epsPos, epsDir, epsMom, epsCov);
}

BOOST_AUTO_TEST_SUITE_END()
