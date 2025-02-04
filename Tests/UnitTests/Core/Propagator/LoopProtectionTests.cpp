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
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StandardAborters.hpp"
#include "Acts/Propagator/detail/LoopProtection.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cmath>
#include <limits>
#include <memory>
#include <numbers>
#include <random>
#include <string>
#include <utility>

namespace bdata = boost::unit_test::data;
using namespace Acts::UnitLiterals;
using namespace Acts::detail;

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();
MagneticFieldContext mfContext = MagneticFieldContext();

/// @brief mockup of stepping state
struct SteppingState {
  /// Parameters
  Vector3 pos = Vector3(0., 0., 0.);
  Vector3 dir = Vector3(0., 0., 1);
  double p = 100_MeV;
};

/// @brief mockup of stepping state
struct Stepper {
  Vector3 field = Vector3(0., 0., 2_T);

  /// Get the field for the stepping, it checks first if the access is still
  /// within the Cell, and updates the cell if necessary.
  ///
  /// @param [in,out] state is the propagation state associated with the track
  ///                 the magnetic field cell is used (and potentially
  ///                 updated)
  /// @param [in] pos is the field position
  Result<Vector3> getField(SteppingState& /*state*/,
                           const Vector3& /*pos*/) const {
    // get the field from the cell
    return Result<Vector3>::success(field);
  }

  /// Access method - position
  Vector3 position(const SteppingState& state) const { return state.pos; }

  /// Access method - direction
  Vector3 direction(const SteppingState& state) const { return state.dir; }

  /// Access method - momentum
  double absoluteMomentum(const SteppingState& state) const { return state.p; }
};

/// @brief mockup of navigation state
struct NavigationState {
  bool navigationBreak = false;
};

/// @brief mockup of the Propagator Options
struct Options {
  /// Absolute maximum path length
  double pathLimit = std::numeric_limits<double>::max();
  bool loopProtection = true;
  double loopFraction = 0.5;
  Direction direction = Direction::Forward();

  bool debug = false;
  std::string debugString;
  int debugMsgWidth = 60;
  int debugPfxWidth = 30;

  /// Contains: target aborters
  ActorList<PathLimitReached> abortList;

  const Acts::Logger& logger = Acts::getDummyLogger();
};

/// @brief mockup of propagtor state
struct PropagatorState {
  /// Contains: stepping state
  SteppingState stepping;
  /// Contains: navigation state
  NavigationState navigation;
  /// Contains: options
  Options options;
};

// This test case checks that no segmentation fault appears
// - this tests the collection of surfaces
BOOST_DATA_TEST_CASE(
    loop_aborter_test,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 21,
                   bdata::distribution = std::uniform_real_distribution<double>(
                       -std::numbers::pi, std::numbers::pi))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 22,
             bdata::distribution = std::uniform_real_distribution<double>(
                 -std::numbers::pi, std::numbers::pi))) ^
        bdata::xrange(1),
    phi, deltaPhi, index) {
  (void)index;
  (void)deltaPhi;

  PropagatorState pState;
  pState.stepping.dir = Vector3(cos(phi), sin(phi), 0.);
  pState.stepping.p = 100_MeV;

  Stepper pStepper;

  auto& pathLimit = pState.options.abortList.get<PathLimitReached>();
  auto initialLimit = pathLimit.internalLimit;

  detail::setupLoopProtection(
      pState, pStepper, pathLimit, false,
      *Acts::getDefaultLogger("LoopProt", Logging::INFO));

  auto updatedLimit =
      pState.options.abortList.get<PathLimitReached>().internalLimit;
  BOOST_CHECK_LT(updatedLimit, initialLimit);
}

using BField = ConstantBField;
using EigenStepper = Acts::EigenStepper<>;
using EigenPropagator = Propagator<EigenStepper>;

const int ntests = 100;
const int skip = 0;

// This test case checks that the propagator with loop LoopProtection
// stops where expected
BOOST_DATA_TEST_CASE(
    propagator_loop_protection_test,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 20,
                   bdata::distribution = std::uniform_real_distribution<double>(
                       0.5_GeV, 10_GeV))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 21,
             bdata::distribution = std::uniform_real_distribution<double>(
                 -std::numbers::pi, std::numbers::pi))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 22,
             bdata::distribution = std::uniform_real_distribution<double>(
                 1., std::numbers::pi - 1.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 23,
                       bdata::distribution =
                           std::uniform_int_distribution<std::uint8_t>(0, 1))) ^
        bdata::xrange(ntests),
    pT, phi, theta, charge, index) {
  if (index < skip) {
    return;
  }

  double px = pT * cos(phi);
  double py = pT * sin(phi);
  double pz = pT / tan(theta);
  double p = pT / sin(theta);
  double q = -1 + 2 * charge;

  const double Bz = 2_T;
  auto bField = std::make_shared<BField>(Vector3{0, 0, Bz});
  EigenStepper estepper(bField);
  EigenPropagator epropagator(std::move(estepper));

  // define start parameters
  BoundTrackParameters start = BoundTrackParameters::makeCurvilinear(
      Vector4(0, 0, 0, 42), phi, theta, q / p, std::nullopt,
      ParticleHypothesis::pion());

  using PropagatorOptions = EigenPropagator::Options<ActorList<>>;
  PropagatorOptions options(tgContext, mfContext);
  options.maxSteps = 1e6;
  const auto& result = epropagator.propagate(start, options).value();

  // this test assumes state.options.loopFraction = 0.5
  CHECK_CLOSE_REL(px, -result.endParameters->momentum().x(), 1e-2);
  CHECK_CLOSE_REL(py, -result.endParameters->momentum().y(), 1e-2);
  CHECK_CLOSE_REL(pz, result.endParameters->momentum().z(), 1e-2);
}

}  // namespace Acts::Test
