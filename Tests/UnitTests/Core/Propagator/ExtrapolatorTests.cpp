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
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/GenericCurvilinearTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/MaterialInteractor.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StandardAborters.hpp"
#include "Acts/Propagator/SurfaceCollector.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Result.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <tuple>
#include <utility>
#include <vector>

namespace bdata = boost::unit_test::data;
using namespace Acts::UnitLiterals;

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();
MagneticFieldContext mfContext = MagneticFieldContext();

// Global definitions
CylindricalTrackingGeometry cGeometry(tgContext);
auto tGeometry = cGeometry();

// Get the navigator and provide the TrackingGeometry
Navigator navigator({tGeometry});

using BFieldType = ConstantBField;
using EigenStepperType = EigenStepper<>;
using EigenPropagatorType = Propagator<EigenStepperType, Navigator>;
using Covariance = BoundSquareMatrix;

auto bField = std::make_shared<BFieldType>(Vector3{0, 0, 2_T});
EigenStepperType estepper(bField);
EigenPropagatorType epropagator(std::move(estepper), std::move(navigator));

const int ntests = 100;

// A plane selector for the SurfaceCollector
struct PlaneSelector {
  /// Call operator
  /// @param sf The input surface to be checked
  bool operator()(const Surface& sf) const {
    return (sf.type() == Surface::Plane);
  }
};

// This test case checks that no segmentation fault appears
// - simple extrapolation test
BOOST_DATA_TEST_CASE(
    test_extrapolation_,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 0,
                   bdata::distribution = std::uniform_real_distribution<double>(
                       0.4_GeV, 10_GeV))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 1,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-M_PI,
                                                                  M_PI))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 2,
             bdata::distribution =
                 std::uniform_real_distribution<double>(1.0, M_PI - 1.0))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 3,
                       bdata::distribution =
                           std::uniform_int_distribution<std::uint8_t>(0, 1))) ^
        bdata::xrange(ntests),
    pT, phi, theta, charge, index) {
  double p = pT / sin(theta);
  double q = -1 + 2 * charge;
  (void)index;

  // define start parameters
  /// a covariance matrix to transport
  Covariance cov;
  // take some major correlations (off-diagonals)
  cov << 10_mm, 0, 0.123, 0, 0.5, 0, 0, 10_mm, 0, 0.162, 0, 0, 0.123, 0, 0.1, 0,
      0, 0, 0, 0.162, 0, 0.1, 0, 0, 0.5, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0,
      0, 0;
  CurvilinearTrackParameters start(Vector4(0, 0, 0, 0), phi, theta, q / p, cov,
                                   ParticleHypothesis::pion());

  EigenPropagatorType::Options<> options(tgContext, mfContext);
  options.stepping.maxStepSize = 10_cm;
  options.pathLimit = 25_cm;

  BOOST_CHECK(
      epropagator.propagate(start, options).value().endParameters.has_value());
}

// This test case checks that no segmentation fault appears
// - this tests the collection of surfaces
BOOST_DATA_TEST_CASE(
    test_surface_collection_,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 10,
                   bdata::distribution = std::uniform_real_distribution<double>(
                       0.4_GeV, 10_GeV))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 11,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-M_PI,
                                                                  M_PI))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 12,
             bdata::distribution =
                 std::uniform_real_distribution<double>(1.0, M_PI - 1.0))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 13,
                       bdata::distribution =
                           std::uniform_int_distribution<std::uint8_t>(0, 1))) ^
        bdata::xrange(ntests),
    pT, phi, theta, charge, index) {
  double p = pT / sin(theta);
  double q = -1 + 2 * charge;
  (void)index;

  // define start parameters
  /// a covariance matrix to transport
  Covariance cov;
  // take some major correlations (off-diagonals)
  cov << 10_mm, 0, 0.123, 0, 0.5, 0, 0, 10_mm, 0, 0.162, 0, 0, 0.123, 0, 0.1, 0,
      0, 0, 0, 0.162, 0, 0.1, 0, 0, 0.5, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0,
      0, 0;
  CurvilinearTrackParameters start(Vector4(0, 0, 0, 0), phi, theta, q / p, cov,
                                   ParticleHypothesis::pion());

  // A PlaneSelector for the SurfaceCollector
  using PlaneCollector = SurfaceCollector<PlaneSelector>;

  EigenPropagatorType::Options<ActorList<PlaneCollector>> options(tgContext,
                                                                  mfContext);

  options.stepping.maxStepSize = 10_cm;
  options.pathLimit = 25_cm;

  const auto& result = epropagator.propagate(start, options).value();
  auto collector_result = result.get<PlaneCollector::result_type>();

  // step through the surfaces and go step by step
  EigenPropagatorType::Options<> optionsEmpty(tgContext, mfContext);

  optionsEmpty.stepping.maxStepSize = 25_cm;
  // Try propagation from start to each surface
  for (const auto& colsf : collector_result.collected) {
    const auto& csurface = colsf.surface;
    // Avoid going to the same surface
    // @todo: decide on strategy and write unit test for this
    if (csurface == &start.referenceSurface()) {
      continue;
    }
    // Extrapolate & check
    const auto& cresult = epropagator.propagate(start, *csurface, optionsEmpty)
                              .value()
                              .endParameters;
    BOOST_CHECK(cresult.has_value());
  }
}

// This test case checks that no segmentation fault appears
// - this tests the collection of surfaces
BOOST_DATA_TEST_CASE(
    test_material_interactor_,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 20,
                   bdata::distribution = std::uniform_real_distribution<double>(
                       0.4_GeV, 10_GeV))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 21,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-M_PI,
                                                                  M_PI))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 22,
             bdata::distribution =
                 std::uniform_real_distribution<double>(1.0, M_PI - 1.0))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 23,
                       bdata::distribution =
                           std::uniform_int_distribution<std::uint8_t>(0, 1))) ^
        bdata::xrange(ntests),
    pT, phi, theta, charge, index) {
  double p = pT / sin(theta);
  double q = -1 + 2 * charge;
  (void)index;

  // define start parameters
  /// a covariance matrix to transport
  Covariance cov;
  // take some major correlations (off-diagonals)
  cov << 10_mm, 0, 0.123, 0, 0.5, 0, 0, 10_mm, 0, 0.162, 0, 0, 0.123, 0, 0.1, 0,
      0, 0, 0, 0.162, 0, 0.1, 0, 0, 0.5, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0,
      0, 0;
  CurvilinearTrackParameters start(Vector4(0, 0, 0, 0), phi, theta, q / p, cov,
                                   ParticleHypothesis::pion());

  EigenPropagatorType::Options<ActorList<MaterialInteractor>> options(
      tgContext, mfContext);
  options.stepping.maxStepSize = 25_cm;
  options.pathLimit = 25_cm;

  const auto& result = epropagator.propagate(start, options).value();
  if (result.endParameters) {
    // test that you actually lost some energy
    BOOST_CHECK_LT(result.endParameters->absoluteMomentum(),
                   start.absoluteMomentum());
  }
}

// This test case checks that no segmentation fault appears
// - this tests the loop protection
BOOST_DATA_TEST_CASE(
    loop_protection_test,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 20,
                   bdata::distribution = std::uniform_real_distribution<double>(
                       0.1_GeV, 0.5_GeV))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 21,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-M_PI,
                                                                  M_PI))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 22,
             bdata::distribution =
                 std::uniform_real_distribution<double>(1.0, M_PI - 1.0))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 23,
                       bdata::distribution =
                           std::uniform_int_distribution<std::uint8_t>(0, 1))) ^
        bdata::xrange(ntests),
    pT, phi, theta, charge, index) {
  double p = pT / sin(theta);
  double q = -1 + 2 * charge;
  (void)index;

  // define start parameters
  /// a covariance matrix to transport
  Covariance cov;
  // take some major correlations (off-diagonals)
  cov << 10_mm, 0, 0.123, 0, 0.5, 0, 0, 10_mm, 0, 0.162, 0, 0, 0.123, 0, 0.1, 0,
      0, 0, 0, 0.162, 0, 0.1, 0, 0, 0.5, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0,
      0, 0;
  CurvilinearTrackParameters start(Vector4(0, 0, 0, 0), phi, theta, q / p, cov,
                                   ParticleHypothesis::pion());

  // Action list and abort list
  EigenPropagatorType::Options<ActorList<MaterialInteractor>> options(
      tgContext, mfContext);
  options.stepping.maxStepSize = 25_cm;
  options.pathLimit = 1500_mm;

  const auto& status = epropagator.propagate(start, options).value();
  // this test assumes state.options.loopFraction = 0.5
  // maximum momentum allowed
  auto bCache = bField->makeCache(mfContext);
  double pmax =
      options.pathLimit *
      bField->getField(start.position(tgContext), bCache).value().norm() / M_PI;
  if (p < pmax) {
    BOOST_CHECK_LT(status.pathLength, options.pathLimit);
  } else {
    CHECK_CLOSE_REL(status.pathLength, options.pathLimit, 1e-3);
  }
}

}  // namespace Acts::Test
