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
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/GenericCurvilinearTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Propagator/AbortList.hpp"
#include "Acts/Propagator/ConstrainedStep.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/EigenStepperDenseExtension.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Propagator/VoidNavigator.hpp"
#include "Acts/Propagator/SurfaceCollector.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/DetectorNavigator.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>
#include <random>
#include <type_traits>
#include <utility>

namespace bdata = boost::unit_test::data;
using namespace Acts::UnitLiterals;
using Acts::VectorHelpers::makeVector4;
using Acts::VectorHelpers::perp;
using namespace Acts::Experimental;

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();
MagneticFieldContext mfContext = MagneticFieldContext();

using Covariance = BoundSquareMatrix;

/// An observer that measures the perpendicular distance
struct PerpendicularMeasure {
  /// Simple result struct to be returned
  struct this_result {
    double distance = std::numeric_limits<double>::max();
  };

  using result_type = this_result;

  PerpendicularMeasure() = default;

  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  void operator()(propagator_state_t& state, const stepper_t& stepper,
                  const navigator_t& /*navigator*/, result_type& result) const {
    result.distance = perp(stepper.position(state.stepping));
  }
};

/// An observer that measures the perpendicular distance
template <typename Surface>
struct SurfaceObserver {
  // the surface to be intersected
  const Surface* surface = nullptr;
  // the tolerance for intersection
  double tolerance = 1e-5;

  /// Simple result struct to be returned
  struct this_result {
    std::size_t surfaces_passed = 0;
    double surface_passed_r = std::numeric_limits<double>::max();
  };

  using result_type = this_result;

  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  void act(propagator_state_t& state, const stepper_t& stepper,
           const navigator_t& /*navigator*/, result_type& result,
           const Logger& /*logger*/) const {
    if (surface == nullptr || result.surfaces_passed != 0) {
      return;
    }

    // calculate the distance to the surface
    const double distance =
        surface
            ->intersect(state.geoContext, stepper.position(state.stepping),
                        stepper.direction(state.stepping),
                        BoundaryTolerance::None())
            .closest()
            .pathLength();

    // Adjust the step size so that we cannot cross the target surface
    state.stepping.stepSize.release(ConstrainedStep::Type::Actor);
    state.stepping.stepSize.update(distance * state.options.direction,
                                   ConstrainedStep::Type::Actor);

    // return true if you fall below tolerance
    if (std::abs(distance) <= tolerance) {
      ++result.surfaces_passed;
      result.surface_passed_r = perp(stepper.position(state.stepping));
      state.stepping.stepSize.release(ConstrainedStep::Type::Actor);
    }
  }
};

struct PlaneSelector {
  /// Call operator
  /// @param sf The input surface to be checked
  bool operator()(const Acts::Surface& sf) const {
    return (sf.type() == Acts::Surface::Plane && sf.geometryId().value() != 0u);
  
  }
};



// Global definitions
using BFieldType = ConstantBField;
using EigenStepperType = EigenStepper<>;
using EigenPropagatorType = Propagator<EigenStepperType>;

const double Bz = 2_T;
auto bField = std::make_shared<BFieldType>(Vector3{0, 0, Bz});
EigenStepperType estepper(bField);
EigenPropagatorType epropagator(std::move(estepper), VoidNavigator(),
                                getDefaultLogger("prop", Logging::VERBOSE));

auto mCylinder = std::make_shared<CylinderBounds>(10_mm, 1000_mm);
auto mSurface =
    Surface::makeShared<CylinderSurface>(Transform3::Identity(), mCylinder);
auto cCylinder = std::make_shared<CylinderBounds>(150_mm, 1000_mm);
auto cSurface =
    Surface::makeShared<CylinderSurface>(Transform3::Identity(), cCylinder);

const int ntests = 5;

// This tests the Options
BOOST_AUTO_TEST_CASE(PropagatorOptions_) {
  using NullOptionsType = EigenPropagatorType::Options<>;
  NullOptionsType null_options(tgContext, mfContext);

  using ActorList = ActorList<PerpendicularMeasure>;
  using OptionsType = EigenPropagatorType::Options<ActorList>;
  OptionsType options(tgContext, mfContext);
}

BOOST_DATA_TEST_CASE(
    cylinder_passage_observer_,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 0,
                   bdata::distribution = std::uniform_real_distribution<double>(
                       0.4_GeV, 10_GeV))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 1,
             bdata::distribution = std::uniform_real_distribution<double>(
                 -std::numbers::pi, std::numbers::pi))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 2,
             bdata::distribution = std::uniform_real_distribution<double>(
                 1., std::numbers::pi - 1.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 3,
                       bdata::distribution =
                           std::uniform_int_distribution<std::uint8_t>(0, 1))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 4,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-1_ns,
                                                                  1_ns))) ^
        bdata::xrange(ntests),
    pT, phi, theta, charge, time, index) {
  double dcharge = -1 + 2 * charge;
  (void)index;

  using CylinderObserver = SurfaceObserver<CylinderSurface>;
  using ActorList = ActorList<CylinderObserver>;

  // setup propagation options
  EigenPropagatorType::Options<ActorList> options(tgContext, mfContext);

  options.pathLimit = 20_m;
  options.stepping.maxStepSize = 1_cm;

  // set the surface to be passed by
  options.actorList.get<CylinderObserver>().surface = mSurface.get();

  using so_result = typename CylinderObserver::result_type;

  // define start parameters
  double x = 0;
  double y = 0;
  double z = 0;
  double px = pT * cos(phi);
  double py = pT * sin(phi);
  double pz = pT / tan(theta);
  double q = dcharge;
  Vector3 pos(x, y, z);
  Vector3 mom(px, py, pz);
  CurvilinearTrackParameters start(makeVector4(pos, time), mom.normalized(),
                                   q / mom.norm(), std::nullopt,
                                   ParticleHypothesis::pion());
  // propagate to the cylinder surface
  const auto& result = epropagator.propagate(start, *cSurface, options).value();
  auto& sor = result.get<so_result>();

  BOOST_CHECK_EQUAL(sor.surfaces_passed, 1u);
  CHECK_CLOSE_ABS(sor.surface_passed_r, 10., 1e-5);
}

BOOST_DATA_TEST_CASE(
    curvilinear_additive_,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 0,
                   bdata::distribution = std::uniform_real_distribution<double>(
                       0.4_GeV, 10_GeV))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 1,
             bdata::distribution = std::uniform_real_distribution<double>(
                 -std::numbers::pi, std::numbers::pi))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 2,
             bdata::distribution = std::uniform_real_distribution<double>(
                 1., std::numbers::pi - 1.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 3,
                       bdata::distribution =
                           std::uniform_int_distribution<std::uint8_t>(0, 1))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 4,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-1_ns,
                                                                  1_ns))) ^
        bdata::xrange(ntests),
    pT, phi, theta, charge, time, index) {
  double dcharge = -1 + 2 * charge;
  (void)index;

  // setup propagation options - the tow step options
  EigenPropagatorType::Options<> options_2s(tgContext, mfContext);
  options_2s.pathLimit = 50_cm;
  options_2s.stepping.maxStepSize = 1_cm;

  // define start parameters
  double x = 0;
  double y = 0;
  double z = 0;
  double px = pT * cos(phi);
  double py = pT * sin(phi);
  double pz = pT / tan(theta);
  double q = dcharge;
  Vector3 pos(x, y, z);
  Vector3 mom(px, py, pz);
  /// a covariance matrix to transport
  Covariance cov;
  // take some major correlations (off-diagonals)
  cov << 10_mm, 0, 0.123, 0, 0.5, 0, 0, 10_mm, 0, 0.162, 0, 0, 0.123, 0, 0.1, 0,
      0, 0, 0, 0.162, 0, 0.1, 0, 0, 0.5, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0,
      0, 0;
  CurvilinearTrackParameters start(makeVector4(pos, time), mom.normalized(),
                                   q / mom.norm(), cov,
                                   ParticleHypothesis::pion());
  // propagate to a path length of 100 with two steps of 50
  const auto& mid_parameters =
      epropagator.propagate(start, options_2s).value().endParameters;
  const auto& end_parameters_2s =
      epropagator.propagate(*mid_parameters, options_2s).value().endParameters;

  // setup propagation options - the one step options
  EigenPropagatorType::Options<> options_1s(tgContext, mfContext);
  options_1s.pathLimit = 100_cm;
  options_1s.stepping.maxStepSize = 1_cm;
  // propagate to a path length of 100 in one step
  const auto& end_parameters_1s =
      epropagator.propagate(start, options_1s).value().endParameters;

  // test that the propagation is additive
  CHECK_CLOSE_REL(end_parameters_1s->position(tgContext),
                  end_parameters_2s->position(tgContext), 0.001);

  BOOST_CHECK(end_parameters_1s->covariance().has_value());
  const auto& cov_1s = *(end_parameters_1s->covariance());
  BOOST_CHECK(end_parameters_2s->covariance().has_value());
  const auto& cov_2s = *(end_parameters_2s->covariance());

  // CHECK_CLOSE_COVARIANCE(cov_1s, cov_2s, 0.001);
  for (unsigned int i = 0; i < cov_1s.rows(); i++) {
    for (unsigned int j = 0; j < cov_1s.cols(); j++) {
      CHECK_CLOSE_OR_SMALL(cov_1s(i, j), cov_2s(i, j), 0.001, 1e-6);
    }
  }
}

BOOST_DATA_TEST_CASE(
    cylinder_additive_,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 0,
                   bdata::distribution = std::uniform_real_distribution<double>(
                       0.4_GeV, 10_GeV))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 1,
             bdata::distribution = std::uniform_real_distribution<double>(
                 -std::numbers::pi, std::numbers::pi))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 2,
             bdata::distribution = std::uniform_real_distribution<double>(
                 1., std::numbers::pi - 1.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 3,
                       bdata::distribution =
                           std::uniform_int_distribution<std::uint8_t>(0, 1))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 4,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-1_ns,
                                                                  1_ns))) ^
        bdata::xrange(ntests),
    pT, phi, theta, charge, time, index) {
  double dcharge = -1 + 2 * charge;
  (void)index;

  // setup propagation options - 2 setp options
  EigenPropagatorType::Options<> options_2s(tgContext, mfContext);
  options_2s.pathLimit = 10_m;
  options_2s.stepping.maxStepSize = 1_cm;

  // define start parameters
  double x = 0;
  double y = 0;
  double z = 0;
  double px = pT * cos(phi);
  double py = pT * sin(phi);
  double pz = pT / tan(theta);
  double q = dcharge;
  Vector3 pos(x, y, z);
  Vector3 mom(px, py, pz);
  /// a covariance matrix to transport
  Covariance cov;
  // take some major correlations (off-diagonals)
  cov << 10_mm, 0, 0.123, 0, 0.5, 0, 0, 10_mm, 0, 0.162, 0, 0, 0.123, 0, 0.1, 0,
      0, 0, 0, 0.162, 0, 0.1, 0, 0, 0.5, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0,
      0, 0;
  CurvilinearTrackParameters start(makeVector4(pos, time), mom.normalized(),
                                   q / mom.norm(), cov,
                                   ParticleHypothesis::pion());
  // propagate to a final surface with one stop in between
  const auto& mid_parameters =
      epropagator.propagate(start, *mSurface, options_2s).value().endParameters;

  const auto& end_parameters_2s =
      epropagator.propagate(*mid_parameters, *cSurface, options_2s)
          .value()
          .endParameters;

  // setup propagation options - one step options
  EigenPropagatorType::Options<> options_1s(tgContext, mfContext);
  options_1s.pathLimit = 10_m;
  options_1s.stepping.maxStepSize = 1_cm;
  // propagate to a final surface in one stop
  const auto& end_parameters_1s =
      epropagator.propagate(start, *cSurface, options_1s).value().endParameters;

  // test that the propagation is additive
  CHECK_CLOSE_REL(end_parameters_1s->position(tgContext),
                  end_parameters_2s->position(tgContext), 0.001);

  BOOST_CHECK(end_parameters_1s->covariance().has_value());
  const auto& cov_1s = (*(end_parameters_1s->covariance()));
  BOOST_CHECK(end_parameters_2s->covariance().has_value());
  const auto& cov_2s = (*(end_parameters_2s->covariance()));

  // CHECK_CLOSE_COVARIANCE(cov_1s, cov_2s, 0.001);
  for (unsigned int i = 0; i < cov_1s.rows(); i++) {
    for (unsigned int j = 0; j < cov_1s.cols(); j++) {
      CHECK_CLOSE_OR_SMALL(cov_1s(i, j), cov_2s(i, j), 0.001, 1e-6);
    }
  }
}

BOOST_AUTO_TEST_CASE(BasicPropagatorInterface) {
  auto field = std::make_shared<ConstantBField>(Vector3{0, 0, 2_T});
  EigenStepper<> eigenStepper{field};
  VoidNavigator navigator{};

  auto startSurface =
      CurvilinearSurface(Vector3::Zero(), Vector3::UnitX()).planeSurface();
  auto targetSurface =
      CurvilinearSurface(Vector3::UnitX() * 20_mm, Vector3::UnitX())
          .planeSurface();

  BoundVector startPars;
  startPars << 0, 0, 0, std::numbers::pi / 2., 1 / 1_GeV, 0;

  BoundTrackParameters startParameters{startSurface, startPars, std::nullopt,
                                       ParticleHypothesis::pion()};

  CurvilinearTrackParameters startCurv{Vector4::Zero(), Vector3::UnitX(),
                                       1. / 1_GeV, std::nullopt,
                                       ParticleHypothesis::pion()};

  GeometryContext gctx;
  MagneticFieldContext mctx;

  {
    EigenPropagatorType::Options<> options{gctx, mctx};

    Propagator propagator{eigenStepper, navigator};
    static_assert(std::is_base_of_v<BasePropagator, decltype(propagator)>,
                  "Propagator does not inherit from BasePropagator");
    const BasePropagator* base =
        static_cast<const BasePropagator*>(&propagator);

    // Ensure the propagation does the same thing
    auto result =
        propagator.propagate(startParameters, *targetSurface, options);
    BOOST_REQUIRE(result.ok());
    BOOST_CHECK_EQUAL(&result.value().endParameters.value().referenceSurface(),
                      targetSurface.get());

    auto resultBase =
        base->propagateToSurface(startParameters, *targetSurface, options);

    BOOST_REQUIRE(resultBase.ok());
    BOOST_CHECK_EQUAL(&resultBase.value().referenceSurface(),
                      targetSurface.get());

    BOOST_CHECK_EQUAL(result.value().endParameters.value().parameters(),
                      resultBase.value().parameters());

    // Propagation call with curvilinear also works
    auto resultCurv =
        base->propagateToSurface(startCurv, *targetSurface, options);
    BOOST_CHECK(resultCurv.ok());
  }

  StraightLineStepper slStepper{};
  {
    Propagator<StraightLineStepper>::Options<> options{gctx, mctx};

    Propagator propagator{slStepper, navigator};
    static_assert(std::is_base_of_v<BasePropagator, decltype(propagator)>,
                  "Propagator does not inherit from BasePropagator");
    const BasePropagator* base =
        static_cast<const BasePropagator*>(&propagator);

    // Ensure the propagation does the same thing
    auto result =
        propagator.propagate(startParameters, *targetSurface, options);
    BOOST_REQUIRE(result.ok());
    BOOST_CHECK_EQUAL(&result.value().endParameters.value().referenceSurface(),
                      targetSurface.get());

    auto resultBase =
        base->propagateToSurface(startParameters, *targetSurface, options);

    BOOST_REQUIRE(resultBase.ok());
    BOOST_CHECK_EQUAL(&resultBase.value().referenceSurface(),
                      targetSurface.get());

    BOOST_CHECK_EQUAL(result.value().endParameters.value().parameters(),
                      resultBase.value().parameters());

    // Propagation call with curvilinear also works
    auto resultCurv =
        base->propagateToSurface(startCurv, *targetSurface, options);
    BOOST_CHECK(resultCurv.ok());
  }

  EigenStepper<EigenStepperDenseExtension> denseEigenStepper{field};

  {
    Propagator propagator{denseEigenStepper, navigator};
    static_assert(!std::is_base_of_v<BasePropagator, decltype(propagator)>,
                  "Propagator unexpectedly inherits from BasePropagator");
  }
}


  //function that checks the covariances if they are rotaed correctly

  void testCovariances(Covariance cov1, Covariance cov2){
  //check if the covariances' for phi,theta,q/p and t remain the same

  BOOST_CHECK((cov1.block<4,4>(2,2)).isApprox(cov2.block<4,4>(2,2)));

  //check if the covariances are rotated
  BOOST_CHECK((cov1.block<2,4>(0,2).row(0)).isApprox(-cov2.block<2,4>(0,2).row(1)));
  BOOST_CHECK((cov1.block<2,4>(0,2).row(1)).isApprox(cov2.block<2,4>(0,2).row(0)));


  BOOST_CHECK((cov1.block<4,2>(2,0).col(0)).isApprox(-cov2.block<4,2>(2,0).col(1)));
  BOOST_CHECK((cov1.block<4,2>(2,0).col(1)).isApprox(cov2.block<4,2>(2,0).col(0)));

  auto subcov1 = cov1.block<2,2>(0,0);
  auto subcov2 = cov2.block<2,2>(0,0);
  BOOST_CHECK(subcov1(0,0) == subcov2(1,1));
  BOOST_CHECK(subcov1(1,1) == subcov2(0,0));
  BOOST_CHECK(subcov1(0,1) == -subcov2(1,0));
  BOOST_CHECK(subcov1(1,0) == -subcov2(0,1));

  }

//check what the propagator does with two surfaces on the same position rotated by 90deg
BOOST_AUTO_TEST_CASE(PlaneSurfacesOnSamePositionPropagationTest){



    
    using ActorList = ActorList<SurfaceCollector<PlaneSelector>, EndOfWorldReached>;

    //Setup the two surfaces and rotate them by 90 degrees on the same plane
    Acts::Transform3 transform2 = Acts::Transform3::Identity();
    transform2.prerotate(Acts::AngleAxis3(0.5*M_PI, Acts::Vector3::UnitZ()));

    auto surface1 = Surface::makeShared<PlaneSurface>(Transform3::Identity(), std::make_shared<RectangleBounds>(1000_mm, 1000_mm));
    auto surface2 = Surface::makeShared<PlaneSurface>(transform2, std::make_shared<RectangleBounds>(1000_mm, 1000_mm));

    surface1->assignGeometryId(Acts::GeometryIdentifier{}.setSensitive(1));
    surface2->assignGeometryId(Acts::GeometryIdentifier{}.setSensitive(2));

    // Define the start parameters for propagattion
    double x = 0.1;
    double y = 0.2;
    double z = -20.;
    double px = 0.1;
    double py = 0.1;
    double pz = 20.;
    double q = 1;
    double absMom = sqrt(px*px + py*py + pz*pz);
    Vector3 pos(x, y, z);
    Vector3 mom(px, py, pz);
    Covariance cov;
      // Take some correlations for the covariance matrix
    cov << 10.5_mm, 0, 0.123, 0, 0.5, 0,
           0, 6.7_mm, 0, 0.162, 0, 0,
           0.123, 0, 0.1, 0, 0, 0,
           0, 0.162, 0, 0.1, 0, 0,
           0.5, 0, 0, 0, 1. / (10.2_GeV), 0,
           0, 0, 0, 0, 0, 0;

    CurvilinearTrackParameters startParams(makeVector4(pos, 0), mom.normalized(), q / absMom,
                                cov, ParticleHypothesis::pion());
   
    // Set the stepper for the propagator with a magnetic field
    auto field = std::make_shared<ConstantBField>(Vector3{0, 0, 0});
   

    {

      //first check "force propagation" to each surface separately

    auto stepper = Acts::EigenStepper(field);
   
    VoidNavigator navigator{};
    EigenPropagatorType::Options<ActorList> options(tgContext, mfContext);
    EigenPropagatorType propagator(std::move(stepper), navigator,
                                getDefaultLogger("Propagator_Test1", Logging::VERBOSE));
  



    // Propagate to each surface seperately
    auto result1 = propagator.propagateToSurface(startParams, *surface1, options);

    auto result2 = propagator.propagateToSurface(startParams, *surface2, options);

    BOOST_CHECK(result1.ok());

    BOOST_CHECK(result2.ok());

    //check the covariances
    auto endParams1 = *result1;
    BoundSquareMatrix cov1 = endParams1.covariance().value();

    auto endParams2 = *result2;
    BoundSquareMatrix cov2 = endParams2.covariance().value();


    //check the covariances

    testCovariances(cov1, cov2);


    }

    //check also full propagation case to surfaces put in a volume

    {
      using PropagatorType = Propagator<EigenStepperType, DetectorNavigator>;

      //create a volume and put the surfaces inside -start propagate from the volume inside

      auto bounds = std::make_unique<CuboidVolumeBounds>(2000_mm, 2000_mm, 2000_mm);

      auto testVolume = DetectorVolumeFactory::construct(
      defaultPortalAndSubPortalGenerator(), tgContext,
      "Detector_Volume",
      Acts::Transform3(Acts::Transform3::Identity()),
      std::move(bounds), std::vector<std::shared_ptr<Acts::Surface>>{surface1, surface2}, 
      std::vector<std::shared_ptr<DetectorVolume>>{},
      tryNoVolumes(),
      tryAllPortalsAndSurfaces());

      testVolume->assignGeometryId(
      Acts::GeometryIdentifier{}.setVolume(1));

     
      auto testDetector = Detector::makeShared(
        "Detector", {testVolume}, tryRootVolumes());

      DetectorNavigator::Config navCfg;
      navCfg.detector = testDetector.get();

      DetectorNavigator detNavigator(navCfg);

      auto stepper = Acts::EigenStepper(field);

      PropagatorType::Options<ActorList> options(tgContext, mfContext);
      PropagatorType propagator(std::move(stepper), detNavigator,
                                getDefaultLogger("Propagator_Test2", Logging::VERBOSE));
   
      auto result = propagator.propagate(startParams, options);

      BOOST_CHECK(result.ok());

      //check if the surfaces are collected during the propagation and if the covariances are rotated
      auto cSurfaces = result.value().get<Acts::SurfaceCollector<PlaneSelector>::result_type>();
      //two plane surfaces should be collected
      BOOST_CHECK(cSurfaces.collected.size() == 2); 
      testCovariances(cSurfaces.collected[0].covariance, cSurfaces.collected[1].covariance);   

      BoundVector startPars;
      startPars << 0.5_mm, 0.5_mm, 0.01, 0.01, 1 / 1_GeV, 0;
      cov << 0.1_mm, 0, 0.123, 0, 0.5, 0,
            0, 0.1_mm, 0, 0.162, 0, 0,
            0.123, 0, 0.1, 0, 0, 0,
            0, 0.162, 0, 0.1, 0, 0,
            0.5, 0, 0, 0, 1. / (10.2_GeV), 0,
            0, 0, 0, 0, 0, 0;

      BoundTrackParameters startParameters{surface1, startPars, cov,
                                         ParticleHypothesis::pion()};   
      auto resultSurf = propagator.propagate(startParameters, options);
      cSurfaces = resultSurf.value().get<Acts::SurfaceCollector<PlaneSelector>::result_type>();
      BOOST_CHECK(resultSurf.ok());
      BOOST_CHECK(cSurfaces.collected.size() == 2);
      testCovariances(cSurfaces.collected[0].covariance, cSurfaces.collected[1].covariance);

    }
}

}  // namespace Acts::Test
