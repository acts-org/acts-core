// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/Propagator/NavigationTarget.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace bdata = boost::unit_test::data;
using namespace Acts::UnitLiterals;
using Acts::VectorHelpers::perp;

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();
MagneticFieldContext mfContext = MagneticFieldContext();

void step(Vector3& pos, const Vector3& dir, double stepSize) {
  pos += stepSize * dir;
}

void step(Vector3& pos, const Vector3& dir, const Surface& surface) {
  auto intersection = surface.intersect(tgContext, pos, dir).closestForward();
  step(pos, dir, intersection.pathLength());
}

void step(Vector3& pos, const Vector3& dir, const NavigationTarget& target) {
  step(pos, dir, *target.surface);
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
/// @param [in] startVol Start volume
/// @param [in] startLay Start layer
/// @param [in] startSurf Start surface
/// @param [in] currSurf Current surface
/// @param [in] currVol Current volume
/// @param [in] targetSurf Target surface
bool testNavigatorStatePointers(Navigator::State& state,
                                const TrackingVolume* startVol,
                                const Layer* startLay, const Surface* startSurf,
                                const Surface* currSurf,
                                const TrackingVolume* currVol,
                                const Surface* targetSurf) {
  std::cout << "startVol: " << startVol << " startLay: " << startLay
            << " startSurf: " << startSurf << " currSurf: " << currSurf
            << " currVol: " << currVol << " targetSurf: " << targetSurf
            << std::endl;

  std::cout << "state.startVolume: " << state.startVolume
            << " state.startLayer: " << state.startLayer
            << " state.startSurface: " << state.startSurface
            << " state.currentSurface: " << state.currentSurface
            << " state.currentVolume: " << state.currentVolume
            << " state.targetSurface: " << state.targetSurface << std::endl;

  return (
      (state.startVolume == startVol) && (state.startLayer == startLay) &&
      (state.startSurface == startSurf) && (state.currentSurface == currSurf) &&
      (state.currentVolume == currVol) && (state.targetSurface == targetSurf));
}

// the surface cache & the creation of the geometry
CylindricalTrackingGeometry cGeometry(tgContext);
auto tGeometry = cGeometry();

const double Bz = 2_T;
auto bField = std::make_shared<ConstantBField>(Vector3{0, 0, Bz});

Acts::Logging::Level logLevel = Acts::Logging::INFO;

BOOST_AUTO_TEST_CASE(Navigator_status_methods) {
  ACTS_LOCAL_LOGGER(Acts::getDefaultLogger("NavigatorTest", logLevel));

  // position and direction vector
  Vector3 position = Vector3::Zero();
  Vector3 direction = Vector3(1., 1., 0).normalized();

  ACTS_INFO("(1) Test for inactivity");
  ACTS_INFO("    a) Run without anything present");
  {
    Navigator::Config navCfg;
    navCfg.resolveSensitive = false;
    navCfg.resolveMaterial = false;
    navCfg.resolvePassive = false;
    Navigator navigator{navCfg};

    Navigator::Options options(tgContext);

    Navigator::State state = navigator.makeState(options);

    BOOST_CHECK(testNavigatorStateVectors(state, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state, nullptr, nullptr, nullptr,
                                           nullptr, nullptr, nullptr));
  }

  ACTS_INFO("    b) Run with geometry but without resolving");
  {
    Navigator::Config navCfg;
    navCfg.resolveSensitive = false;
    navCfg.resolveMaterial = false;
    navCfg.resolvePassive = false;
    navCfg.trackingGeometry = tGeometry;
    Navigator navigator{navCfg};

    Navigator::Options options(tgContext);

    Navigator::State state = navigator.makeState(options);

    BOOST_CHECK(testNavigatorStateVectors(state, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state, nullptr, nullptr, nullptr,
                                           nullptr, nullptr, nullptr));
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

    Navigator::Options options(tgContext);

    Navigator::State state = navigator.makeState(options);

    ACTS_INFO("        i) Because target is reached");
    state.navigationBreak = true;
    navigator.nextTarget(state, position, direction);
    BOOST_CHECK(testNavigatorStateVectors(state, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state, nullptr, nullptr, nullptr,
                                           nullptr, nullptr, nullptr));

    ACTS_INFO("        ii) Because of no target surface");
    state.targetSurface = nullptr;
    navigator.nextTarget(state, position, direction);
    BOOST_CHECK(testNavigatorStateVectors(state, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state, nullptr, nullptr, nullptr,
                                           nullptr, nullptr, nullptr));

    ACTS_INFO("        iii) Because the target surface is reached");
    auto beamline = Surface::makeShared<PerigeeSurface>(Vector3::Zero());
    const Surface* startSurf = beamline.get();
    position = startSurf->center(tgContext);
    const TrackingVolume* startVol =
        tGeometry->lowestTrackingVolume(tgContext, position);
    const Layer* startLay = startVol->associatedLayer(tgContext, position);
    state.options.startSurface = startSurf;
    state.options.targetSurface = startSurf;
    navigator.initialize(state, position, direction, Direction::Forward());
    BOOST_CHECK(testNavigatorStateVectors(state, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state, startVol, startLay, startSurf,
                                           startSurf, startVol, startSurf));

    ACTS_INFO("(2) Test the initialisation");
    ACTS_INFO("    a) Initialise without additional information");
    state = navigator.makeState(options);
    position = Vector3::Zero();
    startVol = tGeometry->lowestTrackingVolume(tgContext, position);
    startLay = startVol->associatedLayer(tgContext, position);
    navigator.initialize(state, position, direction, Direction::Forward());
    BOOST_CHECK(testNavigatorStateVectors(state, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state, startVol, startLay, nullptr,
                                           nullptr, startVol, nullptr));

    ACTS_INFO("    b) Initialise having a start surface");
    state = navigator.makeState(options);
    state.options.startSurface = startSurf;
    navigator.initialize(state, position, direction, Direction::Forward());
    BOOST_CHECK(testNavigatorStateVectors(state, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state, startVol, startLay, startSurf,
                                           startSurf, startVol, nullptr));

    ACTS_INFO("    c) Initialise having a start volume");
    state = navigator.makeState(options);
    state.startVolume = startVol;
    navigator.initialize(state, position, direction, Direction::Forward());
    BOOST_CHECK(testNavigatorStateVectors(state, 0u, 0u, 0u));
    BOOST_CHECK(testNavigatorStatePointers(state, startVol, startLay, nullptr,
                                           nullptr, startVol, nullptr));
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

  Navigator::Options options(tgContext);

  Navigator::State state = navigator.makeState(options);

  // position and direction vector
  Vector3 position = Vector3::Zero();
  Vector3 direction = Vector3(1., 1., 0).normalized();

  // forward navigation ----------------------------------------------
  ACTS_INFO("<<<<<<<<<<<<<<<<<<<<< FORWARD NAVIGATION >>>>>>>>>>>>>>>>>>");

  // (1) Initialization navigation from start point
  // - this will call resolveLayers() as well
  // - and thus should call a return to the stepper
  navigator.initialize(state, position, direction, Direction::Forward());
  // Check that the currentVolume is set
  BOOST_CHECK_NE(state.currentVolume, nullptr);
  // Check that the currentVolume is the startVolume
  BOOST_CHECK_EQUAL(state.currentVolume, state.startVolume);
  // Check that the currentSurface is reset to:
  BOOST_CHECK_EQUAL(state.currentSurface, nullptr);
  // No layer has been found
  BOOST_CHECK_EQUAL(state.navLayers.size(), 0u);

  // Estimate the next target
  NavigationTarget target = navigator.nextTarget(state, position, direction);
  BOOST_CHECK(!target.isNone());
  // A layer has been found
  BOOST_CHECK_EQUAL(state.navLayers.size(), 1u);
  // The index should points to the begin
  BOOST_CHECK_EQUAL(state.navLayerIndex.value(), 0);
  // Check the target is correct
  BOOST_CHECK_EQUAL(target.surface, state.navLayer().first.object());
  // Intersect the target
  auto targetIntersection =
      target.surface->intersect(tgContext, position, direction)
          .closestForward();
  // Cache the beam pipe radius
  double beamPipeR = perp(state.navLayer().first.position());
  // step size has been updated
  CHECK_CLOSE_ABS(targetIntersection.pathLength(), beamPipeR,
                  s_onSurfaceTolerance);

  ACTS_INFO("<<< Test 1a >>> initialize at " << toString(position));

  // Do the step towards the beam pipe
  step(position, direction, target);

  // (2) re-entering navigator:
  // POST STEP
  navigator.handleSurfaceReached(state, position, direction, *target.surface);
  // Check that the currentVolume is the still startVolume
  BOOST_CHECK_EQUAL(state.currentVolume, state.startVolume);
  // The layer number has not changed
  BOOST_CHECK_EQUAL(state.navLayers.size(), 1u);
  // The index still points to the begin
  BOOST_CHECK_EQUAL(state.navLayerIndex.value(), 0);

  ACTS_INFO("<<< Test 1b >>> step to the BeamPipe at  " << toString(position));

  // Estimate the next target
  target = navigator.nextTarget(state, position, direction);
  BOOST_CHECK(!target.isNone());

  // Do the step towards the boundary
  step(position, direction, target);

  // (3) re-entering navigator:
  // POST STEP
  navigator.handleSurfaceReached(state, position, direction, *target.surface);

  ACTS_INFO("<<< Test 1c >>> step to the Boundary at  " << toString(position));

  // Estimate the next target
  target = navigator.nextTarget(state, position, direction);
  BOOST_CHECK(!target.isNone());
  // Intersect the target
  targetIntersection = target.surface->intersect(tgContext, position, direction)
                           .closestForward();

  // positive return: do the step
  step(position, direction, target);

  // (4) re-entering navigator:
  // POST STEP
  navigator.handleSurfaceReached(state, position, direction, *target.surface);

  ACTS_INFO("<<< Test 1d >>> step to 1st layer at  " << toString(position));

  // Estimate the next target
  target = navigator.nextTarget(state, position, direction);
  BOOST_CHECK(!target.isNone());

  // Step through the surfaces on first layer
  for (std::size_t isf = 0; isf < 5; ++isf) {
    step(position, direction, target);
    // (5-9) re-entering navigator:
    // POST STEP
    navigator.handleSurfaceReached(state, position, direction, *target.surface);
    // ACTORS - ABORTERS - PRE STEP
    target = navigator.nextTarget(state, position, direction);
    BOOST_CHECK(!target.isNone());

    ACTS_INFO("<<< Test 1e-1i >>> step within 1st layer at  "
              << toString(position));
  }

  // positive return: do the step
  step(position, direction, target);
  // (10) re-entering navigator:
  // POST STEP
  navigator.handleSurfaceReached(state, position, direction, *target.surface);
  // ACTORS - ABORTERS - PRE STEP
  target = navigator.nextTarget(state, position, direction);
  BOOST_CHECK(!target.isNone());

  ACTS_INFO("<<< Test 1j >>> step to 2nd layer at  " << toString(position));

  // Step through the surfaces on second layer
  for (std::size_t isf = 0; isf < 5; ++isf) {
    step(position, direction, target);
    // (11-15) re-entering navigator:
    // POST STEP
    navigator.handleSurfaceReached(state, position, direction, *target.surface);
    // ACTORS - ABORTERS - PRE STEP
    target = navigator.nextTarget(state, position, direction);
    BOOST_CHECK(!target.isNone());

    ACTS_INFO("<<< Test 1k-1o >>> step within 2nd layer at  "
              << toString(position));
  }

  // positive return: do the step
  step(position, direction, target);
  // (16) re-entering navigator:
  // POST STEP
  navigator.handleSurfaceReached(state, position, direction, *target.surface);
  // ACTORS - ABORTERS - PRE STEP
  target = navigator.nextTarget(state, position, direction);
  BOOST_CHECK(!target.isNone());

  ACTS_INFO("<<< Test 1p >>> step to 3rd layer at  " << toString(position));

  // Step through the surfaces on third layer
  for (std::size_t isf = 0; isf < 3; ++isf) {
    step(position, direction, target);
    // (17-19) re-entering navigator:
    // POST STEP
    navigator.handleSurfaceReached(state, position, direction, *target.surface);
    // ACTORS - ABORTERS - PRE STEP
    target = navigator.nextTarget(state, position, direction);
    BOOST_CHECK(!target.isNone());

    ACTS_INFO("<<< Test 1q-1s >>> step within 3rd layer at  "
              << toString(position));
  }

  // positive return: do the step
  step(position, direction, target);
  // (20) re-entering navigator:
  // POST STEP
  navigator.handleSurfaceReached(state, position, direction, *target.surface);
  // ACTORS - ABORTERS - PRE STEP
  target = navigator.nextTarget(state, position, direction);
  BOOST_CHECK(!target.isNone());

  ACTS_INFO("<<< Test 1t >>> step to 4th layer at  " << toString(position));

  // Step through the surfaces on second layer
  for (std::size_t isf = 0; isf < 3; ++isf) {
    step(position, direction, target);
    // (21-23) re-entering navigator:
    // POST STEP
    navigator.handleSurfaceReached(state, position, direction, *target.surface);
    // ACTORS - ABORTERS - PRE STEP
    target = navigator.nextTarget(state, position, direction);
    BOOST_CHECK(!target.isNone());

    ACTS_INFO("<<< Test 1t-1v >>> step within 4th layer at  "
              << toString(position));
  }

  // positive return: do the step
  step(position, direction, target);
  // (24) re-entering navigator:
  // POST STEP
  navigator.handleSurfaceReached(state, position, direction, *target.surface);
  // ACTORS - ABORTERS - PRE STEP
  target = navigator.nextTarget(state, position, direction);
  BOOST_CHECK(target.isNone());

  ACTS_INFO("<<< Test 1w >>> step to boundary at  " << toString(position));
}

}  // namespace Acts::Test
