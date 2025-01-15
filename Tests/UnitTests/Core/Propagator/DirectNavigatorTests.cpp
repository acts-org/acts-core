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
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Propagator/DirectNavigator.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/MaterialInteractor.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StandardAborters.hpp"
#include "Acts/Propagator/SurfaceCollector.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <numbers>
#include <random>
#include <utility>
#include <vector>

namespace Acts {
class Surface;
}  // namespace Acts

namespace bdata = boost::unit_test::data;
using namespace Acts::UnitLiterals;

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();
MagneticFieldContext mfContext = MagneticFieldContext();

CylindricalTrackingGeometry cGeometry(tgContext);
auto tGeometry = cGeometry();

// Create a navigator for this tracking geometry
Navigator navigator({tGeometry});
DirectNavigator dnavigator;

using BField = ConstantBField;
using Stepper = EigenStepper<>;
using ReferencePropagator = Propagator<Stepper, Navigator>;
using DirectPropagator = Propagator<Stepper, DirectNavigator>;

const double Bz = 2_T;
auto bField = std::make_shared<BField>(Vector3{0, 0, Bz});
Stepper estepper(bField);
Stepper dstepper(bField);

ReferencePropagator rpropagator(std::move(estepper), std::move(navigator));
DirectPropagator dpropagator(std::move(dstepper), std::move(dnavigator));

const int ntests = 1000;
const int skip = 0;
bool referenceTiming = false;
bool oversteppingTest = false;
double oversteppingMaxStepSize = 1_mm;

/// The actual test method that runs the test
/// can be used with several propagator types
///
/// @tparam rpropagator_t is the reference propagator type
/// @tparam dpropagator_t is the direct propagator type
///
/// @param rprop is the reference propagator instance
/// @param dprop is the direct propagator instance
/// @param pT the transverse momentum
/// @param phi the azimuthal angle of the track at creation
/// @param theta the polar angle of the track at creation
/// @param charge is the charge of the particle
/// @param index is the run index from the test
template <typename rpropagator_t, typename dpropagator_t>
void runTest(const rpropagator_t& rprop, const dpropagator_t& dprop, double pT,
             double phi, double theta, int charge, int index) {
  double dcharge = -1 + 2 * charge;

  if (index < skip) {
    return;
  }

  // Define start parameters from ranom input
  double p = pT / sin(theta);
  CurvilinearTrackParameters start(Vector4(0, 0, 0, 0), phi, theta, dcharge / p,
                                   std::nullopt, ParticleHypothesis::pion());

  using EndOfWorld = EndOfWorldReached;

  // Action list and abort list
  using ReferenceActorList =
      ActorList<MaterialInteractor, SurfaceCollector<>, EndOfWorld>;

  // Options definition
  using Options = typename rpropagator_t::template Options<ReferenceActorList>;
  Options pOptions(tgContext, mfContext);
  if (oversteppingTest) {
    pOptions.stepping.maxStepSize = oversteppingMaxStepSize;
  }

  // Surface collector configuration
  auto& sCollector = pOptions.actorList.template get<SurfaceCollector<>>();
  sCollector.selector.selectSensitive = true;
  sCollector.selector.selectMaterial = true;

  // Result is immediately used, non-valid result would indicate failure
  const auto& pResult = rprop.propagate(start, pOptions).value();
  auto& cSurfaces = pResult.template get<SurfaceCollector<>::result_type>();
  auto& cMaterial = pResult.template get<MaterialInteractor::result_type>();
  const Surface& destination = pResult.endParameters->referenceSurface();

  std::cout << " - the standard navigator yielded "
            << cSurfaces.collected.size() << " collected surfaces" << std::endl;

  if (!referenceTiming) {
    // Create the surface sequence
    std::vector<const Surface*> surfaceSequence;
    surfaceSequence.reserve(cSurfaces.collected.size());
    for (auto& cs : cSurfaces.collected) {
      surfaceSequence.push_back(cs.surface);
    }

    // Action list for direct navigator with its initializer
    using DirectActorList = ActorList<MaterialInteractor, SurfaceCollector<>>;

    // Direct options definition
    using DirectOptions =
        typename dpropagator_t::template Options<DirectActorList>;
    DirectOptions dOptions(tgContext, mfContext);
    // Set the surface sequence
    dOptions.navigation.surfaces = surfaceSequence;
    // Surface collector configuration
    auto& dCollector = dOptions.actorList.template get<SurfaceCollector<>>();
    dCollector.selector.selectSensitive = true;
    dCollector.selector.selectMaterial = true;

    // Now redo the propagation with the direct propagator
    const auto& ddResult =
        dprop.propagate(start, destination, dOptions).value();
    auto& ddSurfaces = ddResult.template get<SurfaceCollector<>::result_type>();
    auto& ddMaterial = ddResult.template get<MaterialInteractor::result_type>();

    // CHECK if you have as many surfaces collected as the default navigator
    BOOST_CHECK_EQUAL(cSurfaces.collected.size(), ddSurfaces.collected.size());
    CHECK_CLOSE_REL(cMaterial.materialInX0, ddMaterial.materialInX0, 1e-3);

    // Now redo the propagation with the direct propagator - without destination
    const auto& dwResult = dprop.propagate(start, dOptions).value();
    auto& dwSurfaces = dwResult.template get<SurfaceCollector<>::result_type>();

    // CHECK if you have as many surfaces collected as the default navigator
    BOOST_CHECK_EQUAL(cSurfaces.collected.size(), dwSurfaces.collected.size());
  }
}

// This test case checks that no segmentation fault appears
// - this tests the collection of surfaces
BOOST_DATA_TEST_CASE(
    test_direct_navigator,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 20,
                   bdata::distribution = std::uniform_real_distribution<double>(
                       0.15_GeV, 10_GeV))) ^
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
  // Run the test
  runTest(rpropagator, dpropagator, pT, phi, theta, charge, index);
}

}  // namespace Acts::Test
