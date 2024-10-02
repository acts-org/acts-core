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
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/Propagator/ConstrainedStep.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/StepperConcept.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <tuple>
#include <utility>

namespace Acts {
class Layer;
struct FreeToBoundCorrection;
}  // namespace Acts

namespace bdata = boost::unit_test::data;
using namespace Acts::UnitLiterals;
using Acts::VectorHelpers::perp;

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();
MagneticFieldContext mfContext = MagneticFieldContext();

/// This is a simple cache struct to mimic the
/// Propagator cache
struct PropagatorState {
  /// This is a simple cache struct to mimic a Stepper
  struct Stepper {
    // comply with concept
    using Jacobian = BoundMatrix;
    using Covariance = BoundSquareMatrix;
    using BoundState = std::tuple<BoundTrackParameters, Jacobian, double>;
    using CurvilinearState =
        std::tuple<CurvilinearTrackParameters, Jacobian, double>;
    using BField = int;

    template <typename, typename>
    using return_parameter_type = void;

    /// This is a simple cache struct to mimic the
    /// Stepper cache in the propagation
    struct State {
      /// Position
      Vector4 pos4 = Vector4(0., 0., 0., 0.);

      /// Direction
      Vector3 dir = Vector3(1., 0., 0.);

      /// Momentum
      double p = 0;

      /// Charge
      double q = 0;

      /// Particle hypothesis
      ParticleHypothesis particleHypothesis = ParticleHypothesis::pion();

      // accummulated path length cache
      double pathAccumulated = 0.;

      // adaptive sep size of the runge-kutta integration
      ConstrainedStep stepSize = ConstrainedStep(100_cm);

      // Previous step size for overstep estimation (ignored here)
      double previousStepSize = 0.;

      GeometryContext geoContext = GeometryContext();
    };

    /// State resetter
    void resetState(State& /*state*/, const BoundVector& /*boundParams*/,
                    const BoundSquareMatrix& /*cov*/,
                    const Surface& /*surface*/,
                    const double /*stepSize*/) const {}

    /// Global particle position accessor
    Vector3 position(const State& state) const {
      return state.pos4.segment<3>(Acts::ePos0);
    }

    /// Time access
    double time(const State& state) const { return state.pos4[Acts::eTime]; }

    /// Momentum direction accessor
    Vector3 direction(const State& state) const { return state.dir; }

    /// QoP accessor
    double qOverP(const State& state) const {
      return (state.q == 0 ? 1 : state.q) / state.p;
    }

    /// Absolute momentum accessor
    double absoluteMomentum(const State& state) const { return state.p; }

    /// Momentum accessor
    Vector3 momentum(const State& state) const { return state.p * state.dir; }

    /// Charge access
    double charge(const State& state) const { return state.q; }

    /// Overstep limit access
    double overstepLimit(const State& /*state*/) const {
      return s_onSurfaceTolerance;
    }

    Intersection3D::Status updateSurfaceStatus(
        State& state, const Surface& surface, std::uint8_t index,
        Direction navDir, const BoundaryTolerance& boundaryTolerance,
        ActsScalar surfaceTolerance, const Logger& logger) const {
      return detail::updateSingleSurfaceStatus<Stepper>(
          *this, state, surface, index, navDir, boundaryTolerance,
          surfaceTolerance, logger);
    }

    template <typename object_intersection_t>
    void updateStepSize(State& state,
                        const object_intersection_t& oIntersection,
                        Direction /*direction*/, bool release = true) const {
      detail::updateSingleStepSize<Stepper>(state, oIntersection, release);
    }

    void updateStepSize(State& state, double stepSize,
                        ConstrainedStep::Type stype,
                        bool release = true) const {
      state.previousStepSize = state.stepSize.value();
      state.stepSize.update(stepSize, stype, release);
    }

    double getStepSize(const State& state, ConstrainedStep::Type stype) const {
      return state.stepSize.value(stype);
    }

    void releaseStepSize(State& state, ConstrainedStep::Type stype) const {
      state.stepSize.release(stype);
    }

    std::string outputStepSize(const State& state) const {
      return state.stepSize.toString();
    }

    Result<BoundState> boundState(
        State& state, const Surface& surface, bool /*transportCov*/,
        const FreeToBoundCorrection& /*freeToBoundCorrection*/
    ) const {
      auto bound = BoundTrackParameters::create(
          surface.getSharedPtr(), tgContext, state.pos4, state.dir,
          state.q / state.p, std::nullopt, state.particleHypothesis);
      if (!bound.ok()) {
        return bound.error();
      }
      BoundState bState{std::move(*bound), Jacobian::Identity(),
                        state.pathAccumulated};
      return bState;
    }

    CurvilinearState curvilinearState(State& state, bool /*transportCov*/
    ) const {
      CurvilinearTrackParameters parameters(state.pos4, state.dir,
                                            state.q / state.p, std::nullopt,
                                            state.particleHypothesis);
      // Create the bound state
      CurvilinearState curvState{std::move(parameters), Jacobian::Identity(),
                                 state.pathAccumulated};
      return curvState;
    }

    void update(State& /*state*/, const FreeVector& /*freePars*/,
                const BoundVector& /*boundPars*/, const Covariance& /*cov*/,
                const Surface& /*surface*/) const {}

    void update(State& /*state*/, const Vector3& /*uposition*/,
                const Vector3& /*udirection*/, double /*up*/,
                double /*time*/) const {}

    void transportCovarianceToCurvilinear(State& /*state*/) const {}

    void transportCovarianceToBound(
        State& /*state*/, const Surface& /*surface*/,
        const FreeToBoundCorrection& /*freeToBoundCorrection*/) const {}

    Result<Vector3> getField(State& /*state*/, const Vector3& /*pos*/) const {
      // get the field from the cell
      return Result<Vector3>::success({0., 0., 0.});
    }
  };

  static_assert(StepperConcept<Stepper>,
                "Dummy stepper does not fulfill concept");

  /// emulate the options template
  struct Options {
    Direction direction = Direction::Forward;

    const Acts::Logger& logger = Acts::getDummyLogger();

    ActsScalar surfaceTolerance = s_onSurfaceTolerance;
  };

  /// Navigation cache: the start surface
  const Surface* startSurface = nullptr;

  /// Navigation cache: the current surface
  const Surface* currentSurface = nullptr;

  /// Navigation cache: the target surface
  const Surface* targetSurface = nullptr;
  bool targetReached = false;

  /// Give some options
  Options options;

  /// The Stepper state - internal state of the Stepper
  Stepper::State stepping;

  /// Navigation state - internal state of the Navigator
  Navigator::State navigation;

  // The context cache for this propagation
  GeometryContext geoContext = GeometryContext();
};

template <typename stepper_state_t>
void step(stepper_state_t& sstate) {
  // update the cache position
  sstate.pos4[Acts::ePos0] += sstate.stepSize.value() * sstate.dir[Acts::eMom0];
  sstate.pos4[Acts::ePos1] += sstate.stepSize.value() * sstate.dir[Acts::eMom1];
  sstate.pos4[Acts::ePos2] += sstate.stepSize.value() * sstate.dir[Acts::eMom2];
  // create navigation parameters
  return;
}

/// @brief Method for testing vectors in @c Navigator::State
///
/// @param [in] state Navigator state
/// @param [in] navSurf Number of navigation surfaces
/// @param [in] navLay Number of navigation layers
/// @param [in] navBound Number of navigation boundaries
/// @param [in] extSurf Number of external surfaces
bool testNavigatorStateVectors(Navigator::State& state, std::size_t navSurf,
                               std::size_t navLay, std::size_t navBound) {
  return ((state.navSurfaces.size() == navSurf) &&
          (state.navLayers.size() == navLay) &&
          (state.navBoundaries.size() == navBound));
}

/// @brief Method for testing pointers in @c Navigator::State
///
/// @param [in] state Navigation state
/// @param [in] worldVol World volume
/// @param [in] startVol Start volume
/// @param [in] startLay Start layer
/// @param [in] startSurf Start surface
/// @param [in] currSurf Current surface
/// @param [in] currVol Current volume
/// @param [in] targetVol Target volume
/// @param [in] targetLay Target layer
/// @param [in] targetSurf Target surface
bool testNavigatorStatePointers(Navigator::State& state,
                                const TrackingVolume* worldVol,
                                const TrackingVolume* startVol,
                                const Layer* startLay, const Surface* startSurf,
                                const Surface* currSurf,
                                const TrackingVolume* currVol,
                                const Surface* targetSurf) {
  return ((state.worldVolume == worldVol) && (state.startVolume == startVol) &&
          (state.startLayer == startLay) && (state.startSurface == startSurf) &&
          (state.currentSurface == currSurf) &&
          (state.currentVolume == currVol) &&
          (state.targetSurface == targetSurf));
}
// the surface cache & the creation of the geometry

CylindricalTrackingGeometry cGeometry(tgContext);
auto tGeometry = cGeometry();

const double Bz = 2_T;
auto bField = std::make_shared<ConstantBField>(Vector3{0, 0, Bz});

Acts::Logging::Level logLevel = Acts::Logging::INFO;

BOOST_AUTO_TEST_CASE(Navigator_status_methods) {
  ACTS_LOCAL_LOGGER(Acts::getDefaultLogger("NavigatorTest", logLevel))

  // position and direction vector
  Vector4 position4(0., 0., 0, 0);
  Vector3 momentum(1., 1., 0);

  // the propagator cache
  PropagatorState state;

  // the stepper cache
  state.stepping.pos4 = position4;
  state.stepping.dir = momentum.normalized();

  // Stepper
  PropagatorState::Stepper stepper;

  ACTS_INFO("(1) Test for inactivity");
  ACTS_INFO("    a) Run without anything present");
  {
    Navigator::Config navCfg;
    navCfg.resolveSensitive = false;
    navCfg.resolveMaterial = false;
    navCfg.resolvePassive = false;
    Navigator navigator{navCfg};

    navigator.postStep(state, stepper);
    BOOST_CHECK(testNavigatorStateVectors(state.navigation, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state.navigation, nullptr, nullptr,
                                           nullptr, nullptr, nullptr, nullptr,
                                           nullptr));
  }

  ACTS_INFO("    b) Run with geometry but without resolving");
  {
    Navigator::Config navCfg;
    navCfg.resolveSensitive = false;
    navCfg.resolveMaterial = false;
    navCfg.resolvePassive = false;
    navCfg.trackingGeometry = tGeometry;
    Navigator navigator{navCfg};

    navigator.postStep(state, stepper);
    BOOST_CHECK(testNavigatorStateVectors(state.navigation, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state.navigation, nullptr, nullptr,
                                           nullptr, nullptr, nullptr, nullptr,
                                           nullptr));
  }

  ACTS_INFO(
      "    c) Run with geometry and resolving but broken navigation for "
      "various reasons");
  {
    Navigator::Config navCfg;
    navCfg.resolveSensitive = true;
    navCfg.resolveMaterial = true;
    navCfg.resolvePassive = true;
    navCfg.trackingGeometry = tGeometry;
    Navigator navigator{navCfg};

    state.navigation.navigationBreak = true;
    ACTS_INFO("        i) Because target is reached");
    state.navigation.targetReached = true;
    navigator.postStep(state, stepper);
    BOOST_CHECK(testNavigatorStateVectors(state.navigation, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state.navigation, nullptr, nullptr,
                                           nullptr, nullptr, nullptr, nullptr,
                                           nullptr));

    ACTS_INFO("        ii) Because of no target surface");
    state.navigation.targetReached = false;
    state.navigation.targetSurface = nullptr;
    navigator.postStep(state, stepper);
    BOOST_CHECK(testNavigatorStateVectors(state.navigation, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state.navigation, nullptr, nullptr,
                                           nullptr, nullptr, nullptr, nullptr,
                                           nullptr));
    ACTS_INFO("        iii) Because the target surface is reached");

    auto beamline = Surface::makeShared<PerigeeSurface>(Vector3::Zero());
    const Surface* startSurf = beamline.get();
    state.stepping.pos4.segment<3>(Acts::ePos0) =
        startSurf->center(state.geoContext);
    const Surface* targetSurf = startSurf;
    state.navigation.targetSurface = targetSurf;
    navigator.postStep(state, stepper);
    BOOST_CHECK(testNavigatorStateVectors(state.navigation, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state.navigation, nullptr, nullptr,
                                           nullptr, nullptr, targetSurf,
                                           nullptr, targetSurf));

    ACTS_INFO("(2) Test the initialisation");
    ACTS_INFO("    a) Initialise without additional information");
    state.navigation = Navigator::State();
    state.stepping.pos4 << 0., 0., 0., 0.;
    const TrackingVolume* worldVol = tGeometry->highestTrackingVolume();
    const TrackingVolume* startVol = tGeometry->lowestTrackingVolume(
        state.geoContext, stepper.position(state.stepping));
    const Layer* startLay = startVol->associatedLayer(
        state.geoContext, stepper.position(state.stepping));
    navigator.initialize(state, stepper);
    BOOST_CHECK(testNavigatorStateVectors(state.navigation, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state.navigation, worldVol, startVol,
                                           startLay, nullptr, nullptr, startVol,
                                           nullptr));

    ACTS_INFO("    b) Initialise having a start surface");
    state.navigation = Navigator::State();
    state.navigation.startSurface = startSurf;
    navigator.initialize(state, stepper);
    BOOST_CHECK(testNavigatorStateVectors(state.navigation, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state.navigation, worldVol, startVol,
                                           startLay, startSurf, startSurf,
                                           startVol, nullptr));

    ACTS_INFO("    c) Initialise having a start volume");
    state.navigation = Navigator::State();
    state.navigation.startVolume = startVol;
    navigator.initialize(state, stepper);
    BOOST_CHECK(testNavigatorStateVectors(state.navigation, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state.navigation, worldVol, startVol,
                                           startLay, nullptr, nullptr, startVol,
                                           nullptr));
  }
}

BOOST_AUTO_TEST_CASE(Navigator_target_methods) {
  ACTS_LOCAL_LOGGER(Acts::getDefaultLogger("NavigatorTest", logLevel))

  // create a navigator
  Navigator::Config navCfg;
  navCfg.trackingGeometry = tGeometry;
  navCfg.resolveSensitive = true;
  navCfg.resolveMaterial = true;
  navCfg.resolvePassive = false;
  Navigator navigator{navCfg};

  // position and direction vector
  Vector4 position4(0., 0., 0, 0);
  Vector3 momentum(1., 1., 0);

  // the propagator cache
  PropagatorState state;

  // the stepper cache
  state.stepping.pos4 = position4;
  state.stepping.dir = momentum.normalized();

  // forward navigation ----------------------------------------------
  ACTS_INFO("<<<<<<<<<<<<<<<<<<<<< FORWARD NAVIGATION >>>>>>>>>>>>>>>>>>");

  // Stepper
  PropagatorState::Stepper stepper;

  // (1) Initialization navigation from start point
  // - this will call resolveLayers() as well
  // - and thus should call a return to the stepper
  navigator.initialize(state, stepper);
  // Check that the currentVolume is set
  BOOST_CHECK_NE(state.navigation.currentVolume, nullptr);
  // Check that the currentVolume is the startVolume
  BOOST_CHECK_EQUAL(state.navigation.currentVolume,
                    state.navigation.startVolume);
  // Check that the currentSurface is reset to:
  BOOST_CHECK_EQUAL(state.navigation.currentSurface, nullptr);
  // No layer has been found
  BOOST_CHECK_EQUAL(state.navigation.navLayers.size(), 0u);
  // ACTORS-ABORTERS-TARGET
  navigator.preStep(state, stepper);
  // A layer has been found
  BOOST_CHECK_EQUAL(state.navigation.navLayers.size(), 1u);
  // The index should points to the begin
  BOOST_CHECK_EQUAL(state.navigation.navLayerIndex, 0);
  // Cache the beam pipe radius
  double beamPipeR = perp(state.navigation.navLayer().first.position());
  // step size has been updated
  CHECK_CLOSE_ABS(state.stepping.stepSize.value(), beamPipeR,
                  s_onSurfaceTolerance);

  ACTS_INFO("<<< Test 1a >>> initialize at " << toString(state.stepping.pos4));

  // Do the step towards the beam pipe
  step(state.stepping);

  // (2) re-entering navigator:
  // POST STEP
  navigator.postStep(state, stepper);
  // Check that the currentVolume is the still startVolume
  BOOST_CHECK_EQUAL(state.navigation.currentVolume,
                    state.navigation.startVolume);
  // The layer number has not changed
  BOOST_CHECK_EQUAL(state.navigation.navLayers.size(), 1u);
  // The index still points to the begin
  BOOST_CHECK_EQUAL(state.navigation.navLayerIndex, 0);
  // ACTORS - ABORTERS - PRE STEP
  navigator.preStep(state, stepper);

  ACTS_INFO("<<< Test 1b >>> step to the BeamPipe at  "
            << toString(state.stepping.pos4));

  // Do the step towards the boundary
  step(state.stepping);

  // (3) re-entering navigator:
  // POST STEP
  navigator.postStep(state, stepper);
  // ACTORS - ABORTERS - PRE STEP
  navigator.preStep(state, stepper);

  ACTS_INFO("<<< Test 1c >>> step to the Boundary at  "
            << toString(state.stepping.pos4));

  // positive return: do the step
  step(state.stepping);
  // (4) re-entering navigator:
  // POST STEP
  navigator.postStep(state, stepper);
  // ACTORS - ABORTERS - PRE STEP
  navigator.preStep(state, stepper);

  ACTS_INFO("<<< Test 1d >>> step to 1st layer at  "
            << toString(state.stepping.pos4));

  // Step through the surfaces on first layer
  for (std::size_t isf = 0; isf < 5; ++isf) {
    step(state.stepping);
    // (5-9) re-entering navigator:
    // POST STEP
    navigator.postStep(state, stepper);
    // ACTORS - ABORTERS - PRE STEP
    navigator.preStep(state, stepper);

    ACTS_INFO("<<< Test 1e-1i >>> step within 1st layer at  "
              << toString(state.stepping.pos4));
  }

  // positive return: do the step
  step(state.stepping);
  // (10) re-entering navigator:
  // POST STEP
  navigator.postStep(state, stepper);
  // ACTORS - ABORTERS - PRE STEP
  navigator.preStep(state, stepper);

  ACTS_INFO("<<< Test 1j >>> step to 2nd layer at  "
            << toString(state.stepping.pos4));

  // Step through the surfaces on second layer
  for (std::size_t isf = 0; isf < 5; ++isf) {
    step(state.stepping);
    // (11-15) re-entering navigator:
    // POST STEP
    navigator.postStep(state, stepper);
    // ACTORS - ABORTERS - PRE STEP
    navigator.preStep(state, stepper);

    ACTS_INFO("<<< Test 1k-1o >>> step within 2nd layer at  "
              << toString(state.stepping.pos4));
  }

  // positive return: do the step
  step(state.stepping);
  // (16) re-entering navigator:
  // POST STEP
  navigator.postStep(state, stepper);
  // ACTORS - ABORTERS - PRE STEP
  navigator.preStep(state, stepper);

  ACTS_INFO("<<< Test 1p >>> step to 3rd layer at  "
            << toString(state.stepping.pos4));

  // Step through the surfaces on third layer
  for (std::size_t isf = 0; isf < 3; ++isf) {
    step(state.stepping);
    // (17-19) re-entering navigator:
    // POST STEP
    navigator.postStep(state, stepper);
    // ACTORS - ABORTERS - PRE STEP
    navigator.preStep(state, stepper);

    ACTS_INFO("<<< Test 1q-1s >>> step within 3rd layer at  "
              << toString(state.stepping.pos4));
  }

  // positive return: do the step
  step(state.stepping);
  // (20) re-entering navigator:
  // POST STEP
  navigator.postStep(state, stepper);
  // ACTORS - ABORTERS - PRE STEP
  navigator.preStep(state, stepper);

  ACTS_INFO("<<< Test 1t >>> step to 4th layer at  "
            << toString(state.stepping.pos4));

  // Step through the surfaces on second layer
  for (std::size_t isf = 0; isf < 3; ++isf) {
    step(state.stepping);
    // (21-23) re-entering navigator:
    // POST STEP
    navigator.postStep(state, stepper);
    // ACTORS - ABORTERS - PRE STEP
    navigator.preStep(state, stepper);

    ACTS_INFO("<<< Test 1t-1v >>> step within 4th layer at  "
              << toString(state.stepping.pos4));
  }

  // positive return: do the step
  step(state.stepping);
  // (24) re-entering navigator:
  // POST STEP
  navigator.postStep(state, stepper);
  // ACTORS - ABORTERS - PRE STEP
  navigator.preStep(state, stepper);

  ACTS_INFO("<<< Test 1w >>> step to boundary at  "
            << toString(state.stepping.pos4));
}

}  // namespace Acts::Test
