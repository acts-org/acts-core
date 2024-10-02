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
#include "Acts/EventData/Charge.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/MultiComponentTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/detail/CorrectedTransformationFreeToBound.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/NullBField.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/EigenStepperDefaultExtension.hpp"
#include "Acts/Propagator/MultiEigenStepperLoop.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace Acts {
namespace Concepts {
template <typename T>
concept has_components = requires { typename T::components; };
}  // namespace Concepts

struct MultiStepperSurfaceReached;
}  // namespace Acts

using namespace Acts;
using namespace Acts::VectorHelpers;

/////////////////////////////////////////////////////
// Some useful global objects, typedefs and structs
/////////////////////////////////////////////////////
const MagneticFieldContext magCtx;
const GeometryContext geoCtx;

using MultiStepperLoop = MultiEigenStepperLoop<EigenStepperDefaultExtension>;
using SingleStepper = EigenStepper<EigenStepperDefaultExtension>;

const double defaultStepSize = 123.;
const auto defaultNDir = Direction::Backward;

const auto defaultBField =
    std::make_shared<ConstantBField>(Vector3(1., 2.5, 33.33));
const auto defaultNullBField = std::make_shared<NullBField>();

const auto particleHypothesis = ParticleHypothesis::pion();

struct Options {
  Direction direction = defaultNDir;

  const Acts::Logger &logger = Acts::getDummyLogger();

  struct {
    double stepTolerance = 1e-4;
    double stepSizeCutOff = 0.0;
    std::size_t maxRungeKuttaStepTrials = 10;
  } stepping;
};

struct MockNavigator {};

static constexpr MockNavigator mockNavigator;

struct Navigation {};

template <typename stepper_state_t>
struct DummyPropState {
  stepper_state_t &stepping;
  Options options;
  Navigation navigation;
  GeometryContext geoContext;

  DummyPropState(Direction direction, stepper_state_t &ss)
      : stepping(ss),
        options(Options{}),
        navigation(Navigation{}),
        geoContext(geoCtx) {
    options.direction = direction;
  }
};

// Makes random bound parameters and covariance and a plane surface at {0,0,0}
// with normal {1,0,0}. Optionally some external fixed bound parameters can be
// supplied
auto makeDefaultBoundPars(bool cov = true, std::size_t n = 4,
                          std::optional<BoundVector> ext_pars = std::nullopt) {
  std::vector<std::tuple<double, BoundVector, std::optional<BoundSquareMatrix>>>
      cmps;
  using Opt = std::optional<BoundSquareMatrix>;

  auto make_random_sym_matrix = []() {
    auto c = BoundSquareMatrix::Random().eval();
    c *= c.transpose();
    return c;
  };

  for (auto i = 0ul; i < n; ++i) {
    cmps.push_back({1. / n, ext_pars ? *ext_pars : BoundVector::Random(),
                    cov ? Opt{make_random_sym_matrix()} : Opt{}});
  }

  auto surface = Acts::CurvilinearSurface(Vector3::Zero(), Vector3{1., 0., 0.})
                     .planeSurface();

  return MultiComponentBoundTrackParameters(surface, cmps, particleHypothesis);
}

//////////////////////
/// Test the reducers
//////////////////////
BOOST_AUTO_TEST_CASE(test_max_weight_reducer) {
  // Can use this multistepper since we only care about the state which is
  // invariant
  using MultiState = typename MultiStepperLoop::State;

  constexpr std::size_t N = 4;
  const auto multi_pars = makeDefaultBoundPars(false, N);
  MultiState state(geoCtx, magCtx, defaultBField, multi_pars, defaultStepSize);
  SingleStepper singleStepper(defaultBField);

  double w = 0.1;
  double wSum = 0.0;
  for (auto &[sstate, weight, _] : state.components) {
    weight = w;
    wSum += w;
    w += 0.1;
  }
  BOOST_CHECK_EQUAL(wSum, 1.0);
  BOOST_CHECK_EQUAL(state.components.back().weight, 0.4);

  MaxWeightReducerLoop reducer{};
  BOOST_CHECK_EQUAL(reducer.position(state),
                    singleStepper.position(state.components.back().state));
  BOOST_CHECK_EQUAL(reducer.direction(state),
                    singleStepper.direction(state.components.back().state));
}

BOOST_AUTO_TEST_CASE(test_max_momentum_reducer) {
  // Can use this multistepper since we only care about the state which is
  // invariant
  using MultiState = typename MultiStepperLoop::State;

  constexpr std::size_t N = 4;
  const auto multi_pars = makeDefaultBoundPars(false, N);
  MultiState state(geoCtx, magCtx, defaultBField, multi_pars, defaultStepSize);
  SingleStepper singleStepper(defaultBField);

  double p = 1.0;
  double q = 1.0;
  for (auto &[sstate, weight, _] : state.components) {
    sstate.pars[eFreeQOverP] = q / p;
    p *= 2.0;
  }
  BOOST_CHECK_EQUAL(state.components.back().state.pars[eFreeQOverP], q / 8.0);

  MaxMomentumReducerLoop reducer{};
  BOOST_CHECK_EQUAL(reducer.position(state),
                    singleStepper.position(state.components.back().state));
  BOOST_CHECK_EQUAL(reducer.direction(state),
                    singleStepper.direction(state.components.back().state));
}

//////////////////////////////////////////////////////
/// Test the construction of the MultiStepper::State
//////////////////////////////////////////////////////
template <typename multi_stepper_t, bool Cov>
void test_multi_stepper_state() {
  using MultiState = typename multi_stepper_t::State;
  using MultiStepper = multi_stepper_t;

  constexpr std::size_t N = 4;
  const auto multi_pars = makeDefaultBoundPars(Cov, N, BoundVector::Ones());

  MultiState state(geoCtx, magCtx, defaultBField, multi_pars, defaultStepSize);

  MultiStepper ms(defaultBField);

  BOOST_CHECK_EQUAL(N, ms.numberComponents(state));

  // Test the result & compare with the input/test for reasonable members
  auto const_iterable = ms.constComponentIterable(state);
  for (const auto cmp : const_iterable) {
    BOOST_CHECK_EQUAL(cmp.jacTransport(), FreeMatrix::Identity());
    BOOST_CHECK_EQUAL(cmp.derivative(), FreeVector::Zero());
    if constexpr (!Cov) {
      BOOST_CHECK_EQUAL(cmp.jacToGlobal(), BoundToFreeMatrix::Zero());
      BOOST_CHECK_EQUAL(cmp.cov(), BoundSquareMatrix::Zero());
    }
  }

  BOOST_CHECK_EQUAL(state.pathAccumulated, 0.);
  for (const auto cmp : const_iterable) {
    BOOST_CHECK_EQUAL(cmp.pathAccumulated(), 0.);
  }

  // covTransport in the MultiEigenStepperLoop is redundant and
  // thus not part of the interface. However, we want to check them for
  // consistency.
  if constexpr (Acts::Concepts::has_components<MultiState>) {
    BOOST_CHECK(!state.covTransport);
    for (const auto &cmp : state.components) {
      BOOST_CHECK_EQUAL(cmp.state.covTransport, Cov);
    }
  }
}

BOOST_AUTO_TEST_CASE(multi_stepper_state_no_cov) {
  test_multi_stepper_state<MultiStepperLoop, false>();
}

template <typename multi_stepper_t>
void test_multi_stepper_state_invalid() {
  using MultiState = typename multi_stepper_t::State;

  // Empty component vector
  const auto multi_pars = makeDefaultBoundPars(false, 0);

  BOOST_CHECK_THROW(
      MultiState(geoCtx, magCtx, defaultBField, multi_pars, defaultStepSize),
      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(multi_eigen_stepper_state_invalid) {
  test_multi_stepper_state_invalid<MultiStepperLoop>();
}

////////////////////////////////////////////////////////////////////////
// Compare the Multi-Stepper against the Eigen-Stepper for consistency
////////////////////////////////////////////////////////////////////////
template <typename multi_stepper_t>
void test_multi_stepper_vs_eigen_stepper() {
  using MultiState = typename multi_stepper_t::State;
  using MultiStepper = multi_stepper_t;

  const BoundVector pars = BoundVector::Ones();
  const BoundSquareMatrix cov = BoundSquareMatrix::Identity();

  std::vector<std::tuple<double, BoundVector, std::optional<BoundSquareMatrix>>>
      cmps(4, {0.25, pars, cov});

  auto surface =
      Acts::CurvilinearSurface(Vector3::Zero(), Vector3::Ones().normalized())
          .planeSurface();

  MultiComponentBoundTrackParameters multi_pars(surface, cmps,
                                                particleHypothesis);
  BoundTrackParameters single_pars(surface, pars, cov, particleHypothesis);

  MultiState multi_state(geoCtx, magCtx, defaultBField, multi_pars,
                         defaultStepSize);
  SingleStepper::State single_state(geoCtx, defaultBField->makeCache(magCtx),
                                    single_pars, defaultStepSize);

  MultiStepper multi_stepper(defaultBField);
  SingleStepper single_stepper(defaultBField);

  for (auto cmp : multi_stepper.componentIterable(multi_state)) {
    cmp.status() = Acts::Intersection3D::Status::reachable;
  }

  // Do some steps and check that the results match
  for (int i = 0; i < 10; ++i) {
    // Single stepper
    auto single_prop_state = DummyPropState(defaultNDir, single_state);
    auto single_result = single_stepper.step(single_prop_state, mockNavigator);
    single_stepper.transportCovarianceToCurvilinear(single_state);

    // Multi stepper;
    auto multi_prop_state = DummyPropState(defaultNDir, multi_state);
    auto multi_result = multi_stepper.step(multi_prop_state, mockNavigator);
    multi_stepper.transportCovarianceToCurvilinear(multi_state);

    // Check equality
    BOOST_REQUIRE(multi_result.ok());
    BOOST_REQUIRE_EQUAL(multi_result.ok(), single_result.ok());

    BOOST_CHECK_EQUAL(*single_result, *multi_result);

    for (const auto cmp : multi_stepper.constComponentIterable(multi_state)) {
      BOOST_CHECK_EQUAL(cmp.pars(), single_state.pars);
      BOOST_CHECK_EQUAL(cmp.cov(), single_state.cov);
      BOOST_CHECK_EQUAL(cmp.jacTransport(), single_state.jacTransport);
      BOOST_CHECK_EQUAL(cmp.jacToGlobal(), single_state.jacToGlobal);
      BOOST_CHECK_EQUAL(cmp.derivative(), single_state.derivative);
      BOOST_CHECK_EQUAL(cmp.pathAccumulated(), single_state.pathAccumulated);
    }
  }
}

BOOST_AUTO_TEST_CASE(multi_eigen_vs_single_eigen) {
  test_multi_stepper_vs_eigen_stepper<MultiStepperLoop>();
}

/////////////////////////////
// Test stepsize accessors
/////////////////////////////

// TODO do this later, when we introduce the MultiEigenStepperSIMD, which there
// needs new interfaces...

////////////////////////////////////////////////////
// Test the modifying accessors to the components
////////////////////////////////////////////////////
template <typename multi_stepper_t>
void test_components_modifying_accessors() {
  using MultiState = typename multi_stepper_t::State;
  using MultiStepper = multi_stepper_t;

  const auto multi_pars = makeDefaultBoundPars();

  MultiState mutable_multi_state(geoCtx, magCtx, defaultBField, multi_pars,
                                 defaultStepSize);
  const MultiState const_multi_state(geoCtx, magCtx, defaultBField, multi_pars,
                                     defaultStepSize);

  MultiStepper multi_stepper(defaultBField);

  auto modify = [&](const auto &projector) {
    // Here test the mutable overloads of the mutable iterable
    for (auto cmp : multi_stepper.componentIterable(mutable_multi_state)) {
      using type = std::decay_t<decltype(projector(cmp))>;
      if constexpr (std::is_enum_v<type>) {
        projector(cmp) =
            static_cast<type>(static_cast<int>(projector(cmp)) + 1);
      } else {
        projector(cmp) *= 2.0;
      }
    }
  };

  auto check = [&](const auto &projector) {
    // Here test the const-member functions of the mutable iterable
    auto mutable_state_iterable =
        multi_stepper.componentIterable(mutable_multi_state);
    // Here test the const iterable
    auto const_state_iterable =
        multi_stepper.constComponentIterable(const_multi_state);

    auto mstate_it = mutable_state_iterable.begin();
    auto cstate_it = const_state_iterable.begin();
    for (; cstate_it != const_state_iterable.end(); ++mstate_it, ++cstate_it) {
      const auto mstate_cmp = *mstate_it;
      auto cstate_cmp = *cstate_it;

      using type = std::decay_t<decltype(projector(mstate_cmp))>;

      if constexpr (std::is_arithmetic_v<type>) {
        BOOST_CHECK_CLOSE(projector(mstate_cmp), 2.0 * projector(cstate_cmp),
                          1.e-8);
      } else if constexpr (std::is_enum_v<type>) {
        BOOST_CHECK_EQUAL(static_cast<int>(projector(mstate_cmp)),
                          1 + static_cast<int>(projector(cstate_cmp)));
      } else {
        BOOST_CHECK(
            projector(mstate_cmp).isApprox(2.0 * projector(cstate_cmp), 1.e-8));
      }
    }
  };

  const auto projectors = std::make_tuple(
      [](auto &cmp) -> decltype(auto) { return cmp.status(); },
      [](auto &cmp) -> decltype(auto) { return cmp.pathAccumulated(); },
      [](auto &cmp) -> decltype(auto) { return cmp.weight(); },
      [](auto &cmp) -> decltype(auto) { return cmp.pars(); },
      [](auto &cmp) -> decltype(auto) { return cmp.cov(); },
      [](auto &cmp) -> decltype(auto) { return cmp.jacTransport(); },
      [](auto &cmp) -> decltype(auto) { return cmp.derivative(); },
      [](auto &cmp) -> decltype(auto) { return cmp.jacobian(); },
      [](auto &cmp) -> decltype(auto) { return cmp.jacToGlobal(); });

  std::apply(
      [&](const auto &...projs) {
        // clang-format off
        ( [&]() { modify(projs); check(projs); }(), ...);
        // clang-format on
      },
      projectors);
}

BOOST_AUTO_TEST_CASE(multi_eigen_component_iterable_with_modification) {
  test_components_modifying_accessors<MultiStepperLoop>();
}

/////////////////////////////////////////////
// Test if the surface status update works
/////////////////////////////////////////////
template <typename multi_stepper_t>
void test_multi_stepper_surface_status_update() {
  using MultiState = typename multi_stepper_t::State;
  using MultiStepper = multi_stepper_t;

  auto start_surface =
      Acts::CurvilinearSurface(Vector3::Zero(), Vector3{1.0, 0.0, 0.0})
          .planeSurface();

  auto right_surface =
      Acts::CurvilinearSurface(Vector3{1.0, 0.0, 0.0}, Vector3{1.0, 0.0, 0.0})
          .planeSurface();

  std::vector<std::tuple<double, BoundVector, std::optional<BoundSquareMatrix>>>
      cmps(2, {0.5, BoundVector::Zero(), std::nullopt});
  std::get<BoundVector>(cmps[0])[eBoundTheta] = M_PI_2;
  std::get<BoundVector>(cmps[1])[eBoundTheta] = -M_PI_2;
  std::get<BoundVector>(cmps[0])[eBoundQOverP] = 1.0;
  std::get<BoundVector>(cmps[1])[eBoundQOverP] = 1.0;

  MultiComponentBoundTrackParameters multi_pars(start_surface, cmps,
                                                particleHypothesis);

  BOOST_REQUIRE(std::get<1>(multi_pars[0])
                    .direction()
                    .isApprox(Vector3{1.0, 0.0, 0.0}, 1.e-10));
  BOOST_REQUIRE(std::get<1>(multi_pars[1])
                    .direction()
                    .isApprox(Vector3{-1.0, 0.0, 0.0}, 1.e-10));

  MultiState multi_state(geoCtx, magCtx, defaultNullBField, multi_pars,
                         defaultStepSize);
  SingleStepper::State single_state(
      geoCtx, defaultNullBField->makeCache(magCtx), std::get<1>(multi_pars[0]),
      defaultStepSize);

  MultiStepper multi_stepper(defaultNullBField);
  SingleStepper single_stepper(defaultNullBField);

  // Update surface status and check
  {
    auto status = multi_stepper.updateSurfaceStatus(
        multi_state, *right_surface, 0, Direction::Forward,
        BoundaryTolerance::Infinite());

    BOOST_CHECK_EQUAL(status, Intersection3D::Status::reachable);

    auto cmp_iterable = multi_stepper.constComponentIterable(multi_state);
    auto cmp_1 = *cmp_iterable.begin();
    auto cmp_2 = *(++cmp_iterable.begin());

    BOOST_CHECK_EQUAL(cmp_1.status(), Intersection3D::Status::reachable);
    BOOST_CHECK_EQUAL(cmp_2.status(), Intersection3D::Status::reachable);

    BOOST_CHECK_EQUAL(cmp_1.cmp.state.stepSize.value(), 1.0);
    BOOST_CHECK_EQUAL(cmp_2.cmp.state.stepSize.value(), -1.0);
  }

  // Step forward now
  {
    auto multi_prop_state = DummyPropState(Direction::Forward, multi_state);
    multi_stepper.step(multi_prop_state, mockNavigator);

    // Single stepper
    auto single_prop_state = DummyPropState(Direction::Forward, single_state);
    single_stepper.step(single_prop_state, mockNavigator);
  }

  // Update surface status and check again
  {
    auto status = multi_stepper.updateSurfaceStatus(
        multi_state, *right_surface, 0, Direction::Forward,
        BoundaryTolerance::Infinite());

    BOOST_CHECK_EQUAL(status, Intersection3D::Status::onSurface);

    auto cmp_iterable = multi_stepper.constComponentIterable(multi_state);
    auto cmp_1 = *cmp_iterable.begin();
    auto cmp_2 = *(++cmp_iterable.begin());

    BOOST_CHECK_EQUAL(cmp_1.status(), Intersection3D::Status::onSurface);
    BOOST_CHECK_EQUAL(cmp_2.status(), Intersection3D::Status::onSurface);
  }

  // Start surface should be reachable
  {
    auto status = multi_stepper.updateSurfaceStatus(
        multi_state, *start_surface, 0, Direction::Forward,
        BoundaryTolerance::Infinite());

    BOOST_CHECK_EQUAL(status, Intersection3D::Status::reachable);

    auto cmp_iterable = multi_stepper.constComponentIterable(multi_state);
    auto cmp_1 = *cmp_iterable.begin();
    auto cmp_2 = *(++cmp_iterable.begin());

    BOOST_CHECK_EQUAL(cmp_1.status(), Intersection3D::Status::reachable);
    BOOST_CHECK_EQUAL(cmp_2.status(), Intersection3D::Status::reachable);

    BOOST_CHECK_EQUAL(cmp_1.cmp.state.stepSize.value(), -1.0);
    BOOST_CHECK_EQUAL(cmp_2.cmp.state.stepSize.value(), 1.0);
  }
}

BOOST_AUTO_TEST_CASE(test_surface_status_and_cmpwise_bound_state) {
  test_multi_stepper_surface_status_update<MultiStepperLoop>();
}

//////////////////////////////////
// Test Bound state computations
//////////////////////////////////
template <typename multi_stepper_t>
void test_component_bound_state() {
  using MultiState = typename multi_stepper_t::State;
  using MultiStepper = multi_stepper_t;

  auto start_surface =
      Acts::CurvilinearSurface(Vector3::Zero(), Vector3{1.0, 0.0, 0.0})
          .planeSurface();

  auto right_surface =
      Acts::CurvilinearSurface(Vector3{1.0, 0.0, 0.0}, Vector3{1.0, 0.0, 0.0})
          .planeSurface();

  std::vector<std::tuple<double, BoundVector, std::optional<BoundSquareMatrix>>>
      cmps(2, {0.5, BoundVector::Zero(), std::nullopt});
  std::get<BoundVector>(cmps[0])[eBoundTheta] = M_PI_2;
  std::get<BoundVector>(cmps[1])[eBoundTheta] = -M_PI_2;
  std::get<BoundVector>(cmps[0])[eBoundQOverP] = 1.0;
  std::get<BoundVector>(cmps[1])[eBoundQOverP] = 1.0;

  MultiComponentBoundTrackParameters multi_pars(start_surface, cmps,
                                                particleHypothesis);

  BOOST_REQUIRE(std::get<1>(multi_pars[0])
                    .direction()
                    .isApprox(Vector3{1.0, 0.0, 0.0}, 1.e-10));
  BOOST_REQUIRE(std::get<1>(multi_pars[1])
                    .direction()
                    .isApprox(Vector3{-1.0, 0.0, 0.0}, 1.e-10));

  MultiState multi_state(geoCtx, magCtx, defaultNullBField, multi_pars,
                         defaultStepSize);
  SingleStepper::State single_state(
      geoCtx, defaultNullBField->makeCache(magCtx), std::get<1>(multi_pars[0]),
      defaultStepSize);

  MultiStepper multi_stepper(defaultNullBField);
  SingleStepper single_stepper(defaultNullBField);

  // Step forward now
  {
    multi_stepper.updateSurfaceStatus(multi_state, *right_surface, 0,
                                      Direction::Forward,
                                      BoundaryTolerance::Infinite());
    auto multi_prop_state = DummyPropState(Direction::Forward, multi_state);
    multi_stepper.step(multi_prop_state, mockNavigator);

    // Single stepper
    single_stepper.updateSurfaceStatus(single_state, *right_surface, 0,
                                       Direction::Forward,
                                       BoundaryTolerance::Infinite());
    auto single_prop_state = DummyPropState(Direction::Forward, single_state);
    single_stepper.step(single_prop_state, mockNavigator);
  }

  // Check component-wise bound-state
  {
    auto single_bound_state = single_stepper.boundState(
        single_state, *right_surface, true, FreeToBoundCorrection(false));
    BOOST_REQUIRE(single_bound_state.ok());

    auto cmp_iterable = multi_stepper.componentIterable(multi_state);
    auto cmp_1 = *cmp_iterable.begin();
    auto cmp_2 = *(++cmp_iterable.begin());

    auto bound_state_1 =
        cmp_1.boundState(*right_surface, true, FreeToBoundCorrection(false));
    BOOST_REQUIRE(bound_state_1.ok());
    BOOST_CHECK(*single_bound_state == *bound_state_1);

    auto bound_state_2 =
        cmp_2.boundState(*right_surface, true, FreeToBoundCorrection(false));
    BOOST_CHECK(bound_state_2.ok());
  }
}

BOOST_AUTO_TEST_CASE(test_component_wise_bound_state) {
  test_component_bound_state<MultiStepperLoop>();
}

template <typename multi_stepper_t>
void test_combined_bound_state_function() {
  using MultiState = typename multi_stepper_t::State;
  using MultiStepper = multi_stepper_t;

  auto surface =
      Acts::CurvilinearSurface(Vector3::Zero(), Vector3{1.0, 0.0, 0.0})
          .planeSurface();

  // Use Ones() here, so that the angles are in correct range
  const auto pars = BoundVector::Ones().eval();
  const auto cov = []() {
    auto c = BoundSquareMatrix::Random().eval();
    c *= c.transpose();
    return c;
  }();

  std::vector<std::tuple<double, BoundVector, std::optional<BoundSquareMatrix>>>
      cmps(4, {0.25, pars, cov});

  MultiComponentBoundTrackParameters multi_pars(surface, cmps,
                                                particleHypothesis);
  MultiState multi_state(geoCtx, magCtx, defaultBField, multi_pars,
                         defaultStepSize);
  MultiStepper multi_stepper(defaultBField);

  auto res = multi_stepper.boundState(multi_state, *surface, true,
                                      FreeToBoundCorrection(false));

  BOOST_REQUIRE(res.ok());

  const auto [bound_pars, jacobian, pathLength] = *res;

  BOOST_CHECK_EQUAL(jacobian, decltype(jacobian)::Zero());
  BOOST_CHECK_EQUAL(pathLength, 0.0);
  BOOST_CHECK(bound_pars.parameters().isApprox(pars, 1.e-8));
  BOOST_CHECK(bound_pars.covariance()->isApprox(cov, 1.e-8));
}

BOOST_AUTO_TEST_CASE(test_combined_bound_state) {
  test_combined_bound_state_function<MultiStepperLoop>();
}

//////////////////////////////////////////////////
// Test the combined curvilinear state function
//////////////////////////////////////////////////
template <typename multi_stepper_t>
void test_combined_curvilinear_state_function() {
  using MultiState = typename multi_stepper_t::State;
  using MultiStepper = multi_stepper_t;

  auto surface =
      Acts::CurvilinearSurface(Vector3::Zero(), Vector3{1.0, 0.0, 0.0})
          .planeSurface();

  // Use Ones() here, so that the angles are in correct range
  const auto pars = BoundVector::Ones().eval();
  const auto cov = []() {
    auto c = BoundSquareMatrix::Random().eval();
    c *= c.transpose();
    return c;
  }();

  std::vector<std::tuple<double, BoundVector, std::optional<BoundSquareMatrix>>>
      cmps(4, {0.25, pars, cov});
  BoundTrackParameters check_pars(surface, pars, cov, particleHypothesis);

  MultiComponentBoundTrackParameters multi_pars(surface, cmps,
                                                particleHypothesis);
  MultiState multi_state(geoCtx, magCtx, defaultBField, multi_pars,
                         defaultStepSize);
  MultiStepper multi_stepper(defaultBField);

  const auto [curv_pars, jac, pathLength] =
      multi_stepper.curvilinearState(multi_state);

  BOOST_CHECK(
      curv_pars.fourPosition(multi_state.geoContext)
          .isApprox(check_pars.fourPosition(multi_state.geoContext), 1.e-8));
  BOOST_CHECK(curv_pars.direction().isApprox(check_pars.direction(), 1.e-8));
  BOOST_CHECK_CLOSE(curv_pars.absoluteMomentum(), check_pars.absoluteMomentum(),
                    1.e-8);
  BOOST_CHECK_CLOSE(curv_pars.charge(), check_pars.charge(), 1.e-8);
}

BOOST_AUTO_TEST_CASE(test_curvilinear_state) {
  test_combined_curvilinear_state_function<MultiStepperLoop>();
}

////////////////////////////////////
// Test single component interface
////////////////////////////////////

template <typename multi_stepper_t>
void test_single_component_interface_function() {
  using MultiState = typename multi_stepper_t::State;
  using MultiStepper = multi_stepper_t;

  std::vector<std::tuple<double, BoundVector, std::optional<BoundSquareMatrix>>>
      cmps;
  for (int i = 0; i < 4; ++i) {
    cmps.push_back({0.25, BoundVector::Random(), BoundSquareMatrix::Random()});
  }

  auto surface =
      Acts::CurvilinearSurface(Vector3::Zero(), Vector3::Ones().normalized())
          .planeSurface();

  MultiComponentBoundTrackParameters multi_pars(surface, cmps,
                                                particleHypothesis);

  MultiState multi_state(geoCtx, magCtx, defaultBField, multi_pars,
                         defaultStepSize);

  MultiStepper multi_stepper(defaultBField);

  DummyPropState multi_prop_state(defaultNDir, multi_state);

  // Check at least some properties at the moment
  auto check = [&](auto cmp) {
    auto sstepper = cmp.singleStepper(multi_stepper);
    auto &sstepping = cmp.singleState(multi_prop_state).stepping;

    BOOST_CHECK_EQUAL(sstepper.position(sstepping),
                      cmp.pars().template segment<3>(eFreePos0));
    BOOST_CHECK_EQUAL(sstepper.direction(sstepping),
                      cmp.pars().template segment<3>(eFreeDir0));
    BOOST_CHECK_EQUAL(sstepper.time(sstepping), cmp.pars()[eFreeTime]);
    BOOST_CHECK_CLOSE(sstepper.qOverP(sstepping), cmp.pars()[eFreeQOverP],
                      1.e-8);
  };

  for (const auto cmp : multi_stepper.constComponentIterable(multi_state)) {
    check(cmp);
  }

  for (auto cmp : multi_stepper.componentIterable(multi_state)) {
    check(cmp);
  }
}

BOOST_AUTO_TEST_CASE(test_single_component_interface) {
  test_single_component_interface_function<MultiStepperLoop>();
}

//////////////////////////////
// Remove and add components
//////////////////////////////

template <typename multi_stepper_t>
void remove_add_components_function() {
  using MultiState = typename multi_stepper_t::State;
  using MultiStepper = multi_stepper_t;

  const auto multi_pars = makeDefaultBoundPars(4);

  MultiState multi_state(geoCtx, magCtx, defaultBField, multi_pars,
                         defaultStepSize);

  MultiStepper multi_stepper(defaultBField);

  {
    BoundTrackParameters pars(multi_pars.referenceSurface().getSharedPtr(),
                              BoundVector::Ones(), std::nullopt,
                              particleHypothesis);
    multi_stepper.addComponent(multi_state, pars, 0.0);
  }

  BOOST_CHECK_EQUAL(multi_stepper.numberComponents(multi_state),
                    multi_pars.components().size() + 1);

  multi_stepper.clearComponents(multi_state);

  BOOST_CHECK_EQUAL(multi_stepper.numberComponents(multi_state), 0);
}

BOOST_AUTO_TEST_CASE(remove_add_components_test) {
  remove_add_components_function<MultiStepperLoop>();
}

//////////////////////////////////////////////////
// Instantiate a Propagator with the MultiStepper
//////////////////////////////////////////////////

template <typename multi_stepper_t>
void propagator_instatiation_test_function() {
  auto bField = std::make_shared<NullBField>();
  multi_stepper_t multi_stepper(bField);
  Propagator<multi_stepper_t, Navigator> propagator(
      std::move(multi_stepper), Navigator{Navigator::Config{}});

  auto surface =
      Acts::CurvilinearSurface(Vector3::Zero(), Vector3{1.0, 0.0, 0.0})
          .planeSurface();
  using PropagatorOptions =
      typename Propagator<multi_stepper_t, Navigator>::template Options<>;
  PropagatorOptions options(geoCtx, magCtx);

  std::vector<std::tuple<double, BoundVector, std::optional<BoundSquareMatrix>>>
      cmps(4, {0.25, BoundVector::Ones().eval(),
               BoundSquareMatrix::Identity().eval()});
  MultiComponentBoundTrackParameters pars(surface, cmps, particleHypothesis);

  // This only checks that this compiles, not that it runs without errors
  // @TODO: Add test that checks the target aborter works correctly

  // Instantiate with target
  using type_a =
      decltype(propagator.template propagate<decltype(pars), decltype(options),
                                             MultiStepperSurfaceReached>(
          pars, *surface, options));
  static_assert(!std::is_same_v<type_a, void>);

  // Instantiate without target
  using type_b = decltype(propagator.propagate(pars, options));
  static_assert(!std::is_same_v<type_b, void>);
}

BOOST_AUTO_TEST_CASE(propagator_instatiation_test) {
  propagator_instatiation_test_function<MultiStepperLoop>();
}
