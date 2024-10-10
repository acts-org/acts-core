// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/Charge.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/GenericCurvilinearTrackParameters.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/TransformationHelpers.hpp"
#include "Acts/Geometry/BoundarySurfaceT.hpp"
#include "Acts/Geometry/CuboidVolumeBuilder.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingGeometryBuilder.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/MagneticField/NullBField.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousVolumeMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Propagator/ConstrainedStep.hpp"
#include "Acts/Propagator/MaterialInteractor.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/SympyStepper.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/UnitVectors.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace Acts {
class ISurfaceMaterial;
class Logger;
}  // namespace Acts

using namespace Acts::UnitLiterals;
using Acts::VectorHelpers::makeVector4;

namespace Acts::Test {

using Covariance = BoundSquareMatrix;

static constexpr auto eps = 3 * std::numeric_limits<double>::epsilon();

// Create a test context
GeometryContext tgContext = GeometryContext();
MagneticFieldContext mfContext = MagneticFieldContext();

/// @brief Simplified propagator state
template <typename stepper_state_t>
struct PropState {
  /// @brief Constructor
  PropState(Direction direction, stepper_state_t sState)
      : stepping(std::move(sState)) {
    options.direction = direction;
  }
  /// State of the sympy stepper
  stepper_state_t stepping;
  /// Propagator options which only carry the relevant components
  struct {
    Direction direction = Direction::Forward;
    struct {
      double stepTolerance = 1e-4;
      double stepSizeCutOff = 0.;
      unsigned int maxRungeKuttaStepTrials = 10000;
    } stepping;
  } options;
};

struct MockNavigator {};

static constexpr MockNavigator mockNavigator;

/// @brief Aborter for the case that a particle leaves the detector or reaches
/// a custom made threshold.
///
struct EndOfWorld {
  /// Maximum value in x-direction of the detector
  double maxX = 1_m;

  /// @brief Constructor
  EndOfWorld() = default;

  /// @brief Main call operator for the abort operation
  ///
  /// @tparam propagator_state_t State of the propagator
  /// @tparam stepper_t Type of the stepper
  /// @tparam navigator_t Type of the navigator
  ///
  /// @param [in] state State of the propagation
  /// @param [in] stepper Stepper of the propagation
  /// @param [in] navigator Navigator of the propagation
  ///
  /// @return Boolean statement if the particle is still in the detector
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  bool operator()(propagator_state_t& state, const stepper_t& stepper,
                  const navigator_t& /*navigator*/,
                  const Logger& /*logger*/) const {
    const double tolerance = state.options.surfaceTolerance;
    if (maxX - std::abs(stepper.position(state.stepping).x()) <= tolerance ||
        std::abs(stepper.position(state.stepping).y()) >= 0.5_m ||
        std::abs(stepper.position(state.stepping).z()) >= 0.5_m) {
      return true;
    }
    return false;
  }
};

///
/// @brief Data collector while propagation
///
struct StepCollector {
  ///
  /// @brief Data container for result analysis
  ///
  struct this_result {
    // Position of the propagator after each step
    std::vector<Vector3> position;
    // Momentum of the propagator after each step
    std::vector<Vector3> momentum;
  };

  using result_type = this_result;

  /// @brief Main call operator for the action list. It stores the data for
  /// analysis afterwards
  ///
  /// @tparam propagator_state_t Type of the propagator state
  /// @tparam stepper_t Type of the stepper
  /// @tparam navigator_t Type of the navigator
  ///
  /// @param [in] state State of the propagator
  /// @param [in] stepper Stepper of the propagation
  /// @param [in] navigator Navigator of the propagation
  /// @param [out] result Struct which is filled with the data
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  void operator()(propagator_state_t& state, const stepper_t& stepper,
                  const navigator_t& /*navigator*/, result_type& result,
                  const Logger& /*logger*/) const {
    result.position.push_back(stepper.position(state.stepping));
    result.momentum.push_back(stepper.momentum(state.stepping));
  }
};

/// These tests are aiming to test whether the state setup is working properly
BOOST_AUTO_TEST_CASE(sympy_stepper_state_test) {
  // Set up some variables
  double stepSize = 123.;
  auto bField = std::make_shared<ConstantBField>(Vector3(1., 2.5, 33.33));

  Vector3 pos(1., 2., 3.);
  Vector3 dir(4., 5., 6.);
  double time = 7.;
  double absMom = 8.;
  double charge = -1.;

  // Test charged parameters without covariance matrix
  CurvilinearTrackParameters cp(makeVector4(pos, time), dir, charge / absMom,
                                std::nullopt, ParticleHypothesis::pion());
  SympyStepper::State esState(tgContext, bField->makeCache(mfContext), cp,
                              stepSize);

  SympyStepper es(bField);

  // Test the result & compare with the input/test for reasonable members
  BOOST_CHECK_EQUAL(esState.jacToGlobal, BoundToFreeMatrix::Zero());
  BOOST_CHECK_EQUAL(esState.jacTransport, FreeMatrix::Identity());
  BOOST_CHECK_EQUAL(esState.derivative, FreeVector::Zero());
  BOOST_CHECK(!esState.covTransport);
  BOOST_CHECK_EQUAL(esState.cov, Covariance::Zero());
  BOOST_CHECK_EQUAL(esState.pathAccumulated, 0.);
  BOOST_CHECK_EQUAL(esState.stepSize.value(), stepSize);
  BOOST_CHECK_EQUAL(esState.previousStepSize, 0.);

  // Test without charge and covariance matrix
  CurvilinearTrackParameters ncp(makeVector4(pos, time), dir, 1 / absMom,
                                 std::nullopt, ParticleHypothesis::pion0());
  esState = SympyStepper::State(tgContext, bField->makeCache(mfContext), ncp,
                                stepSize);
  BOOST_CHECK_EQUAL(es.charge(esState), 0.);

  // Test with covariance matrix
  Covariance cov = 8. * Covariance::Identity();
  ncp = CurvilinearTrackParameters(makeVector4(pos, time), dir, 1 / absMom, cov,
                                   ParticleHypothesis::pion0());
  esState = SympyStepper::State(tgContext, bField->makeCache(mfContext), ncp,
                                stepSize);
  BOOST_CHECK_NE(esState.jacToGlobal, BoundToFreeMatrix::Zero());
  BOOST_CHECK(esState.covTransport);
  BOOST_CHECK_EQUAL(esState.cov, cov);
}

/// These tests are aiming to test the functions of the SympyStepper
/// The numerical correctness of the stepper is tested in the integration tests
BOOST_AUTO_TEST_CASE(sympy_stepper_test) {
  // Set up some variables for the state
  Direction navDir = Direction::Backward;
  double stepSize = 123.;
  auto bField = std::make_shared<ConstantBField>(Vector3(1., 2.5, 33.33));
  auto bCache = bField->makeCache(mfContext);

  // Construct the parameters
  Vector3 pos(1., 2., 3.);
  Vector3 dir = Vector3(4., 5., 6.).normalized();
  double time = 7.;
  double absMom = 8.;
  double charge = -1.;
  Covariance cov = 8. * Covariance::Identity();
  CurvilinearTrackParameters cp(makeVector4(pos, time), dir, charge / absMom,
                                cov, ParticleHypothesis::pion());

  // Build the state and the stepper
  SympyStepper::State esState(tgContext, bField->makeCache(mfContext), cp,
                              stepSize);
  SympyStepper es(bField);

  // Test the getters
  CHECK_CLOSE_ABS(es.position(esState), pos, eps);
  CHECK_CLOSE_ABS(es.direction(esState), dir, eps);
  CHECK_CLOSE_ABS(es.absoluteMomentum(esState), absMom, eps);
  CHECK_CLOSE_ABS(es.charge(esState), charge, eps);
  CHECK_CLOSE_ABS(es.time(esState), time, eps);
  BOOST_CHECK_EQUAL(es.getField(esState, pos).value(),
                    bField->getField(pos, bCache).value());

  // Step size modifies
  const std::string originalStepSize = esState.stepSize.toString();

  es.updateStepSize(esState, -1337., ConstrainedStep::actor);
  BOOST_CHECK_EQUAL(esState.previousStepSize, stepSize);
  BOOST_CHECK_EQUAL(esState.stepSize.value(), -1337.);

  es.releaseStepSize(esState, ConstrainedStep::actor);
  BOOST_CHECK_EQUAL(esState.stepSize.value(), stepSize);
  BOOST_CHECK_EQUAL(es.outputStepSize(esState), originalStepSize);

  // Test the curvilinear state construction
  auto curvState = es.curvilinearState(esState);
  auto curvPars = std::get<0>(curvState);
  CHECK_CLOSE_ABS(curvPars.position(tgContext), cp.position(tgContext), eps);
  CHECK_CLOSE_ABS(curvPars.momentum(), cp.momentum(), 10e-6);
  CHECK_CLOSE_ABS(curvPars.charge(), cp.charge(), eps);
  CHECK_CLOSE_ABS(curvPars.time(), cp.time(), eps);
  BOOST_CHECK(curvPars.covariance().has_value());
  BOOST_CHECK_NE(*curvPars.covariance(), cov);
  CHECK_CLOSE_COVARIANCE(std::get<1>(curvState),
                         BoundMatrix(BoundMatrix::Identity()), eps);
  CHECK_CLOSE_ABS(std::get<2>(curvState), 0., eps);

  // Test the update method
  Vector3 newPos(2., 4., 8.);
  Vector3 newMom(3., 9., 27.);
  double newTime(321.);
  es.update(esState, newPos, newMom.normalized(), charge / newMom.norm(),
            newTime);
  BOOST_CHECK_EQUAL(es.position(esState), newPos);
  BOOST_CHECK_EQUAL(es.direction(esState), newMom.normalized());
  BOOST_CHECK_EQUAL(es.absoluteMomentum(esState), newMom.norm());
  BOOST_CHECK_EQUAL(es.charge(esState), charge);
  BOOST_CHECK_EQUAL(es.time(esState), newTime);

  // The covariance transport
  esState.cov = cov;
  es.transportCovarianceToCurvilinear(esState);
  BOOST_CHECK_NE(esState.cov, cov);
  BOOST_CHECK_NE(esState.jacToGlobal, BoundToFreeMatrix::Zero());
  BOOST_CHECK_EQUAL(esState.jacTransport, FreeMatrix::Identity());
  BOOST_CHECK_EQUAL(esState.derivative, FreeVector::Zero());

  // Perform a step without and with covariance transport
  esState.cov = cov;
  PropState ps(navDir, std::move(esState));

  ps.stepping.covTransport = false;
  es.step(ps, mockNavigator).value();
  CHECK_CLOSE_COVARIANCE(ps.stepping.cov, cov, eps);
  BOOST_CHECK_NE(es.position(ps.stepping).norm(), newPos.norm());
  BOOST_CHECK_NE(es.direction(ps.stepping), newMom.normalized());
  BOOST_CHECK_EQUAL(es.charge(ps.stepping), charge);
  BOOST_CHECK_LT(es.time(ps.stepping), newTime);
  BOOST_CHECK_EQUAL(ps.stepping.derivative, FreeVector::Zero());
  BOOST_CHECK_EQUAL(ps.stepping.jacTransport, FreeMatrix::Identity());

  ps.stepping.covTransport = true;
  es.step(ps, mockNavigator).value();
  CHECK_CLOSE_COVARIANCE(ps.stepping.cov, cov, eps);
  BOOST_CHECK_NE(es.position(ps.stepping).norm(), newPos.norm());
  BOOST_CHECK_NE(es.direction(ps.stepping), newMom.normalized());
  BOOST_CHECK_EQUAL(es.charge(ps.stepping), charge);
  BOOST_CHECK_LT(es.time(ps.stepping), newTime);
  BOOST_CHECK_NE(ps.stepping.derivative, FreeVector::Zero());
  BOOST_CHECK_NE(ps.stepping.jacTransport, FreeMatrix::Identity());

  /// Test the state reset
  // Construct the parameters
  Vector3 pos2(1.5, -2.5, 3.5);
  Vector3 dir2 = Vector3(4.5, -5.5, 6.5).normalized();
  double time2 = 7.5;
  double absMom2 = 8.5;
  double charge2 = 1.;
  BoundSquareMatrix cov2 = 8.5 * Covariance::Identity();
  CurvilinearTrackParameters cp2(makeVector4(pos2, time2), dir2,
                                 charge2 / absMom2, cov2,
                                 ParticleHypothesis::pion());
  FreeVector freeParams = transformBoundToFreeParameters(
      cp2.referenceSurface(), tgContext, cp2.parameters());
  navDir = Direction::Forward;
  double stepSize2 = -2. * stepSize;

  auto copyState = [&](auto& field, const auto& state) {
    using field_t = std::decay_t<decltype(field)>;
    std::decay_t<decltype(state)> copy(tgContext, field.makeCache(mfContext),
                                       cp, stepSize);
    copy.pars = state.pars;
    copy.covTransport = state.covTransport;
    copy.cov = state.cov;
    copy.jacobian = state.jacobian;
    copy.jacToGlobal = state.jacToGlobal;
    copy.jacTransport = state.jacTransport;
    copy.derivative = state.derivative;
    copy.pathAccumulated = state.pathAccumulated;
    copy.stepSize = state.stepSize;
    copy.previousStepSize = state.previousStepSize;

    copy.fieldCache = MagneticFieldProvider::Cache(
        std::in_place_type<typename field_t::Cache>,
        state.fieldCache.template as<typename field_t::Cache>());

    copy.geoContext = state.geoContext;
    copy.stepData = state.stepData;

    return copy;
  };

  // Reset all possible parameters
  SympyStepper::State esStateCopy(copyState(*bField, ps.stepping));
  BOOST_CHECK(cp2.covariance().has_value());
  es.resetState(esStateCopy, cp2.parameters(), *cp2.covariance(),
                cp2.referenceSurface(), stepSize2);
  // Test all components
  BOOST_CHECK_NE(esStateCopy.jacToGlobal, BoundToFreeMatrix::Zero());
  BOOST_CHECK_NE(esStateCopy.jacToGlobal, ps.stepping.jacToGlobal);
  BOOST_CHECK_EQUAL(esStateCopy.jacTransport, FreeMatrix::Identity());
  BOOST_CHECK_EQUAL(esStateCopy.derivative, FreeVector::Zero());
  BOOST_CHECK(esStateCopy.covTransport);
  BOOST_CHECK_EQUAL(esStateCopy.cov, cov2);
  BOOST_CHECK_EQUAL(es.position(esStateCopy),
                    freeParams.template segment<3>(eFreePos0));
  BOOST_CHECK_EQUAL(es.direction(esStateCopy),
                    freeParams.template segment<3>(eFreeDir0).normalized());
  BOOST_CHECK_EQUAL(es.absoluteMomentum(esStateCopy),
                    std::abs(1. / freeParams[eFreeQOverP]));
  BOOST_CHECK_EQUAL(es.charge(esStateCopy), -es.charge(ps.stepping));
  BOOST_CHECK_EQUAL(es.time(esStateCopy), freeParams[eFreeTime]);
  BOOST_CHECK_EQUAL(esStateCopy.pathAccumulated, 0.);
  BOOST_CHECK_EQUAL(esStateCopy.stepSize.value(), navDir * stepSize2);
  BOOST_CHECK_EQUAL(esStateCopy.previousStepSize, ps.stepping.previousStepSize);

  // Reset all possible parameters except the step size
  esStateCopy = copyState(*bField, ps.stepping);
  es.resetState(esStateCopy, cp2.parameters(), *cp2.covariance(),
                cp2.referenceSurface());
  // Test all components
  BOOST_CHECK_NE(esStateCopy.jacToGlobal, BoundToFreeMatrix::Zero());
  BOOST_CHECK_NE(esStateCopy.jacToGlobal, ps.stepping.jacToGlobal);
  BOOST_CHECK_EQUAL(esStateCopy.jacTransport, FreeMatrix::Identity());
  BOOST_CHECK_EQUAL(esStateCopy.derivative, FreeVector::Zero());
  BOOST_CHECK(esStateCopy.covTransport);
  BOOST_CHECK_EQUAL(esStateCopy.cov, cov2);
  BOOST_CHECK_EQUAL(es.position(esStateCopy),
                    freeParams.template segment<3>(eFreePos0));
  BOOST_CHECK_EQUAL(es.direction(esStateCopy),
                    freeParams.template segment<3>(eFreeDir0).normalized());
  BOOST_CHECK_EQUAL(es.absoluteMomentum(esStateCopy),
                    std::abs(1. / freeParams[eFreeQOverP]));
  BOOST_CHECK_EQUAL(es.charge(esStateCopy), -es.charge(ps.stepping));
  BOOST_CHECK_EQUAL(es.time(esStateCopy), freeParams[eFreeTime]);
  BOOST_CHECK_EQUAL(esStateCopy.pathAccumulated, 0.);
  BOOST_CHECK_EQUAL(esStateCopy.stepSize.value(),
                    std::numeric_limits<double>::max());
  BOOST_CHECK_EQUAL(esStateCopy.previousStepSize, ps.stepping.previousStepSize);

  // Reset the least amount of parameters
  esStateCopy = copyState(*bField, ps.stepping);
  es.resetState(esStateCopy, cp2.parameters(), *cp2.covariance(),
                cp2.referenceSurface());
  // Test all components
  BOOST_CHECK_NE(esStateCopy.jacToGlobal, BoundToFreeMatrix::Zero());
  BOOST_CHECK_NE(esStateCopy.jacToGlobal, ps.stepping.jacToGlobal);
  BOOST_CHECK_EQUAL(esStateCopy.jacTransport, FreeMatrix::Identity());
  BOOST_CHECK_EQUAL(esStateCopy.derivative, FreeVector::Zero());
  BOOST_CHECK(esStateCopy.covTransport);
  BOOST_CHECK_EQUAL(esStateCopy.cov, cov2);
  BOOST_CHECK_EQUAL(es.position(esStateCopy),
                    freeParams.template segment<3>(eFreePos0));
  BOOST_CHECK_EQUAL(es.direction(esStateCopy),
                    freeParams.template segment<3>(eFreeDir0).normalized());
  BOOST_CHECK_EQUAL(es.absoluteMomentum(esStateCopy),
                    std::abs(1. / freeParams[eFreeQOverP]));
  BOOST_CHECK_EQUAL(es.charge(esStateCopy), -es.charge(ps.stepping));
  BOOST_CHECK_EQUAL(es.time(esStateCopy), freeParams[eFreeTime]);
  BOOST_CHECK_EQUAL(esStateCopy.pathAccumulated, 0.);
  BOOST_CHECK_EQUAL(esStateCopy.stepSize.value(),
                    std::numeric_limits<double>::max());
  BOOST_CHECK_EQUAL(esStateCopy.previousStepSize, ps.stepping.previousStepSize);

  /// Repeat with surface related methods
  auto plane = CurvilinearSurface(pos, dir.normalized()).planeSurface();
  auto bp = BoundTrackParameters::create(
                plane, tgContext, makeVector4(pos, time), dir, charge / absMom,
                cov, ParticleHypothesis::pion())
                .value();
  esState = SympyStepper::State(tgContext, bField->makeCache(mfContext), cp,
                                stepSize);

  // Test the intersection in the context of a surface
  auto targetSurface =
      CurvilinearSurface(pos + navDir * 2. * dir, dir).planeSurface();
  es.updateSurfaceStatus(esState, *targetSurface, 0, navDir,
                         BoundaryTolerance::None());
  CHECK_CLOSE_ABS(esState.stepSize.value(ConstrainedStep::actor), navDir * 2.,
                  eps);

  // Test the step size modification in the context of a surface
  es.updateStepSize(
      esState,
      targetSurface
          ->intersect(esState.geoContext, es.position(esState),
                      navDir * es.direction(esState), BoundaryTolerance::None())
          .closest(),
      navDir, false);
  CHECK_CLOSE_ABS(esState.stepSize.value(), 2., eps);
  esState.stepSize.setUser(navDir * stepSize);
  es.updateStepSize(
      esState,
      targetSurface
          ->intersect(esState.geoContext, es.position(esState),
                      navDir * es.direction(esState), BoundaryTolerance::None())
          .closest(),
      navDir, true);
  CHECK_CLOSE_ABS(esState.stepSize.value(), 2., eps);

  // Test the bound state construction
  auto boundState = es.boundState(esState, *plane).value();
  auto boundPars = std::get<0>(boundState);
  CHECK_CLOSE_ABS(boundPars.position(tgContext), bp.position(tgContext), eps);
  CHECK_CLOSE_ABS(boundPars.momentum(), bp.momentum(), 1e-7);
  CHECK_CLOSE_ABS(boundPars.charge(), bp.charge(), eps);
  CHECK_CLOSE_ABS(boundPars.time(), bp.time(), eps);
  BOOST_CHECK(boundPars.covariance().has_value());
  BOOST_CHECK_NE(*boundPars.covariance(), cov);
  CHECK_CLOSE_COVARIANCE(std::get<1>(boundState),
                         BoundMatrix(BoundMatrix::Identity()), eps);
  CHECK_CLOSE_ABS(std::get<2>(boundState), 0., eps);

  // Transport the covariance in the context of a surface
  es.transportCovarianceToBound(esState, *plane);
  BOOST_CHECK_NE(esState.cov, cov);
  BOOST_CHECK_NE(esState.jacToGlobal, BoundToFreeMatrix::Zero());
  BOOST_CHECK_EQUAL(esState.jacTransport, FreeMatrix::Identity());
  BOOST_CHECK_EQUAL(esState.derivative, FreeVector::Zero());

  // Update in context of a surface
  freeParams = transformBoundToFreeParameters(bp.referenceSurface(), tgContext,
                                              bp.parameters());

  es.update(esState, freeParams, bp.parameters(), 2 * (*bp.covariance()),
            *plane);
  CHECK_CLOSE_OR_SMALL(es.position(esState), pos, eps, eps);
  CHECK_CLOSE_OR_SMALL(es.direction(esState), dir, eps, eps);
  CHECK_CLOSE_REL(es.absoluteMomentum(esState), absMom, eps);
  BOOST_CHECK_EQUAL(es.charge(esState), charge);
  CHECK_CLOSE_OR_SMALL(es.time(esState), time, eps, eps);
  CHECK_CLOSE_COVARIANCE(esState.cov, Covariance(2. * cov), eps);

  // Test a case where no step size adjustment is required
  ps.options.stepping.stepTolerance = 2. * 4.4258e+09;
  double h0 = esState.stepSize.value();
  es.step(ps, mockNavigator);
  CHECK_CLOSE_ABS(h0, esState.stepSize.value(), eps);
}

}  // namespace Acts::Test
