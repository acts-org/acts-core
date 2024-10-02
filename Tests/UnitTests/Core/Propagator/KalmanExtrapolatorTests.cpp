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
#include "Acts/EventData/GenericCurvilinearTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Tests/CommonHelpers/CubicTrackingGeometry.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Result.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace Acts {
class Logger;
struct EndOfWorldReached;
}  // namespace Acts

using namespace Acts::UnitLiterals;

namespace Acts::Test {

using Jacobian = BoundMatrix;
using Covariance = BoundSquareMatrix;

// Create a test context
GeometryContext tgContext = GeometryContext();
MagneticFieldContext mfContext = MagneticFieldContext();
///
/// @brief the bound state propagation
///
struct StepWiseActor {
  /// The result is the piece-wise jacobian
  struct this_result {
    std::vector<Jacobian> jacobians = {};
    std::vector<double> paths = {};

    double fullPath = 0.;

    bool finalized = false;
  };
  /// Broadcast the result type
  using result_type = this_result;

  /// @brief Kalman sequence operation
  ///
  /// @tparam propagator_state_t is the type of Propagator state
  /// @tparam stepper_t Type of the stepper used for the propagation
  /// @tparam navigator_t Type of the navigator used for the propagation
  ///
  /// @param state is the mutable propagator state object
  /// @param stepper The stepper in use
  /// @param navigator The navigator in use
  /// @param result is the mutable result state object
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  void act(propagator_state_t& state, const stepper_t& stepper,
           const navigator_t& navigator, result_type& result,
           const Logger& /*logger*/) const {
    // Listen to the surface and create bound state where necessary
    auto surface = navigator.currentSurface(state.navigation);
    if (surface && surface->associatedDetectorElement()) {
      // Create a bound state and log the jacobian
      auto boundState = stepper.boundState(state.stepping, *surface).value();
      result.jacobians.push_back(std::move(std::get<Jacobian>(boundState)));
      result.paths.push_back(std::get<double>(boundState));
    }
    // Also store the jacobian and full path
    if (state.stage == PropagatorStage::postPropagation && !result.finalized) {
      // Set the last stepping parameter
      result.paths.push_back(state.stepping.pathAccumulated);
      // Set the full parameter
      result.fullPath = state.stepping.pathAccumulated;
      // Remember that you finalized this
      result.finalized = true;
    }
  }
};

///
/// @brief Unit test for Kalman fitter propagation
///
BOOST_AUTO_TEST_CASE(kalman_extrapolator) {
  // Build detector, take the cubic detector
  CubicTrackingGeometry cGeometry(tgContext);
  auto detector = cGeometry();

  // The Navigator through the detector geometry
  Navigator::Config cfg{detector};
  cfg.resolvePassive = false;
  cfg.resolveMaterial = true;
  cfg.resolveSensitive = true;
  Navigator navigator(cfg);

  // Configure propagation with deactivated B-field
  auto bField = std::make_shared<ConstantBField>(Vector3(0., 0., 0.));
  using Stepper = EigenStepper<>;
  Stepper stepper(bField);
  using Propagator = Propagator<Stepper, Navigator>;
  Propagator propagator(stepper, navigator);

  // Set initial parameters for the particle track
  Covariance cov;
  cov << 10_mm, 0, 0.123, 0, 0.5, 0, 0, 10_mm, 0, 0.162, 0, 0, 0.123, 0, 0.1, 0,
      0, 0, 0, 0.162, 0, 0.1, 0, 0, 0.5, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0,
      0, 0;
  // The start parameters
  CurvilinearTrackParameters start(Vector4(-3_m, 0, 0, 42_ns), 0_degree,
                                   90_degree, 1_e / 1_GeV, cov,
                                   ParticleHypothesis::pion());

  // Create the ActionList and AbortList
  using StepWiseResult = StepWiseActor::result_type;
  using StepWiseActors = ActorList<StepWiseActor, EndOfWorldReached>;

  // Create some options
  using StepWiseOptions = Propagator::Options<StepWiseActors>;
  StepWiseOptions swOptions(tgContext, mfContext);

  using PlainActors = ActorList<EndOfWorldReached>;
  using PlainOptions = Propagator::Options<PlainActors>;
  PlainOptions pOptions(tgContext, mfContext);

  // Run the standard propagation
  const auto& pResult = propagator.propagate(start, pOptions).value();
  // Let's get the end parameters and jacobian matrix
  const auto& pJacobian = *(pResult.transportJacobian);

  // Run the stepwise propagation
  const auto& swResult = propagator.propagate(start, swOptions).value();
  auto swJacobianTest = swResult.template get<StepWiseResult>();

  // (1) Path length test
  double accPath = 0.;
  auto swPaths = swJacobianTest.paths;
  // Sum up the step-wise paths, they are total though
  for (auto cpath = swPaths.rbegin(); cpath != swPaths.rend(); ++cpath) {
    if (cpath != swPaths.rend() - 1) {
      accPath += (*cpath) - (*(cpath + 1));
      continue;
    }
    accPath += (*cpath) - 0.;
  }
  CHECK_CLOSE_REL(swJacobianTest.fullPath, accPath, 1e-6);

  // (2) Jacobian test
  Jacobian accJacobian = Jacobian::Identity();
  // The stepwise jacobians
  auto swJacobians = swJacobianTest.jacobians;
  // The last-step jacobian, needed for the full jacobian transport
  const auto& swlJacobian = *(swResult.transportJacobian);

  // Build up the step-wise jacobians
  for (auto& j : swJacobians) {
    accJacobian = j * accJacobian;
  }
  accJacobian = swlJacobian * accJacobian;
  CHECK_CLOSE_OR_SMALL(pJacobian, accJacobian, 1e-6, 1e-9);
}

}  // namespace Acts::Test
