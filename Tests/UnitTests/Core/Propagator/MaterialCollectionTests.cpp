// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/GenericCurvilinearTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/MaterialInteractor.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StandardAborters.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <numbers>
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

using BField = ConstantBField;
using EigenStepper = Acts::EigenStepper<>;
using EigenPropagator = Propagator<EigenStepper, Navigator>;
using StraightLinePropagator = Propagator<StraightLineStepper, Navigator>;

const double Bz = 2_T;
auto bField = std::make_shared<BField>(Vector3{0, 0, Bz});

EigenStepper estepper(bField);
Navigator esnavigator({tGeometry});
EigenPropagator epropagator(std::move(estepper), std::move(esnavigator));

StraightLineStepper slstepper;
Navigator slnavigator({tGeometry});
StraightLinePropagator slpropagator(slstepper, std::move(slnavigator));

int ntests = 500;
int skip = 0;
bool debugMode = false;

/// the actual test nethod that runs the test can be used with several
/// propagator types
///
/// @tparam propagator_t is the actual propagator type
///
/// @param prop is the propagator instance
/// @param start the start parameters
template <typename propagator_t>
void runTest(const propagator_t& prop,
             const CurvilinearTrackParameters& start) {
  // Action list and abort list
  using ActorList = ActorList<MaterialInteractor>;

  using Options = typename propagator_t::template Options<ActorList>;
  Options fwdOptions(tgContext, mfContext);
  fwdOptions.stepping.maxStepSize = 25_cm;
  fwdOptions.pathLimit = 25_cm;

  // get the material collector and configure it
  auto& fwdMaterialInteractor =
      fwdOptions.actorList.template get<MaterialInteractor>();
  fwdMaterialInteractor.recordInteractions = true;
  fwdMaterialInteractor.energyLoss = false;
  fwdMaterialInteractor.multipleScattering = false;

  if (debugMode) {
    std::cout << ">>> Forward Propagation : start." << std::endl;
  }
  // forward material test
  const auto& fwdResult = prop.propagate(start, fwdOptions).value();
  const auto& fwdMaterial =
      fwdResult.template get<MaterialInteractor::result_type>();
  // check that the collected material is not zero
  BOOST_CHECK_NE(fwdMaterial.materialInX0, 0.);
  BOOST_CHECK_NE(fwdMaterial.materialInL0, 0.);

  double fwdStepMaterialInX0 = 0.;
  double fwdStepMaterialInL0 = 0.;
  // check that the sum of all steps is the total material
  for (auto& mInteraction : fwdMaterial.materialInteractions) {
    fwdStepMaterialInX0 += mInteraction.materialSlab.thicknessInX0();
    fwdStepMaterialInL0 += mInteraction.materialSlab.thicknessInL0();
  }
  CHECK_CLOSE_REL(fwdMaterial.materialInX0, fwdStepMaterialInX0, 1e-3);
  CHECK_CLOSE_REL(fwdMaterial.materialInL0, fwdStepMaterialInL0, 1e-3);

  // get the forward output to the screen
  if (debugMode) {
    // check if the surfaces are free
    std::cout << ">>> Material steps found on ..." << std::endl;
    for (auto& fwdStepsC : fwdMaterial.materialInteractions) {
      std::cout << "--> Surface with " << fwdStepsC.surface->geometryId()
                << std::endl;
    }
  }

  // backward material test
  Options bwdOptions(tgContext, mfContext);
  bwdOptions.stepping.maxStepSize = 25_cm;
  bwdOptions.pathLimit = -25_cm;
  bwdOptions.direction = Direction::Backward();

  // get the material collector and configure it
  auto& bwdMaterialInteractor =
      bwdOptions.actorList.template get<MaterialInteractor>();
  bwdMaterialInteractor.recordInteractions = true;
  bwdMaterialInteractor.energyLoss = false;
  bwdMaterialInteractor.multipleScattering = false;

  const auto& startSurface = start.referenceSurface();

  if (debugMode) {
    std::cout << ">>> Backward Propagation : start." << std::endl;
  }
  const auto& bwdResult =
      prop.propagate(*fwdResult.endParameters, startSurface, bwdOptions)
          .value();
  if (debugMode) {
    std::cout << ">>> Backward Propagation : end." << std::endl;
  }
  const auto& bwdMaterial =
      bwdResult.template get<typename MaterialInteractor::result_type>();
  // check that the collected material is not zero
  BOOST_CHECK_NE(bwdMaterial.materialInX0, 0.);
  BOOST_CHECK_NE(bwdMaterial.materialInL0, 0.);

  double bwdStepMaterialInX0 = 0.;
  double bwdStepMaterialInL0 = 0.;
  // check that the sum of all steps is the total material
  for (auto& mInteraction : bwdMaterial.materialInteractions) {
    bwdStepMaterialInX0 += mInteraction.materialSlab.thicknessInX0();
    bwdStepMaterialInL0 += mInteraction.materialSlab.thicknessInL0();
  }
  CHECK_CLOSE_REL(bwdMaterial.materialInX0, bwdStepMaterialInX0, 1e-3);
  CHECK_CLOSE_REL(bwdMaterial.materialInL0, bwdStepMaterialInL0, 1e-3);

  // get the backward output to the screen
  if (debugMode) {
    // check if the surfaces are free
    std::cout << ">>> Material steps found on ..." << std::endl;
    for (auto& bwdStepsC : bwdMaterial.materialInteractions) {
      std::cout << "--> Surface with " << bwdStepsC.surface->geometryId()
                << std::endl;
    }
  }

  // forward-backward compatibility test
  BOOST_CHECK_EQUAL(bwdMaterial.materialInteractions.size(),
                    fwdMaterial.materialInteractions.size());

  CHECK_CLOSE_REL(bwdMaterial.materialInX0, fwdMaterial.materialInX0, 1e-3);
  CHECK_CLOSE_REL(bwdMaterial.materialInL0, fwdMaterial.materialInL0, 1e-3);

  // stepping from one surface to the next
  // now go from surface to surface and check
  Options fwdStepOptions(tgContext, mfContext);
  fwdStepOptions.stepping.maxStepSize = 25_cm;
  fwdStepOptions.pathLimit = 25_cm;

  // get the material collector and configure it
  auto& fwdStepMaterialInteractor =
      fwdStepOptions.actorList.template get<MaterialInteractor>();
  fwdStepMaterialInteractor.recordInteractions = true;
  fwdStepMaterialInteractor.energyLoss = false;
  fwdStepMaterialInteractor.multipleScattering = false;

  double fwdStepStepMaterialInX0 = 0.;
  double fwdStepStepMaterialInL0 = 0.;

  if (debugMode) {
    // check if the surfaces are free
    std::cout << ">>> Forward steps to be processed sequentially ..."
              << std::endl;
    for (auto& fwdStepsC : fwdMaterial.materialInteractions) {
      std::cout << "--> Surface with " << fwdStepsC.surface->geometryId()
                << std::endl;
    }
  }

  // move forward step by step through the surfaces
  BoundTrackParameters sParameters = start;
  std::vector<BoundTrackParameters> stepParameters;
  for (auto& fwdSteps : fwdMaterial.materialInteractions) {
    if (debugMode) {
      std::cout << ">>> Forward step : "
                << sParameters.referenceSurface().geometryId() << " --> "
                << fwdSteps.surface->geometryId() << std::endl;
    }

    // make a forward step
    const auto& fwdStep =
        prop.propagate(sParameters, (*fwdSteps.surface), fwdStepOptions)
            .value();

    auto& fwdStepMaterial =
        fwdStep.template get<typename MaterialInteractor::result_type>();
    fwdStepStepMaterialInX0 += fwdStepMaterial.materialInX0;
    fwdStepStepMaterialInL0 += fwdStepMaterial.materialInL0;

    if (fwdStep.endParameters.has_value()) {
      // make sure the parameters do not run out of scope
      stepParameters.push_back(*fwdStep.endParameters);
      sParameters = stepParameters.back();
    }
  }
  // final destination surface
  const Surface& dSurface = fwdResult.endParameters->referenceSurface();

  if (debugMode) {
    std::cout << ">>> Forward step : "
              << sParameters.referenceSurface().geometryId() << " --> "
              << dSurface.geometryId() << std::endl;
  }

  const auto& fwdStepFinal =
      prop.propagate(sParameters, dSurface, fwdStepOptions).value();

  auto& fwdStepMaterial =
      fwdStepFinal.template get<typename MaterialInteractor::result_type>();
  fwdStepStepMaterialInX0 += fwdStepMaterial.materialInX0;
  fwdStepStepMaterialInL0 += fwdStepMaterial.materialInL0;

  // forward-forward step compatibility test
  CHECK_CLOSE_REL(fwdStepStepMaterialInX0, fwdStepMaterialInX0, 1e-3);
  CHECK_CLOSE_REL(fwdStepStepMaterialInL0, fwdStepMaterialInL0, 1e-3);

  // stepping from one surface to the next : backwards
  // now go from surface to surface and check
  Options bwdStepOptions(tgContext, mfContext);
  bwdStepOptions.stepping.maxStepSize = 25_cm;
  bwdStepOptions.pathLimit = -25_cm;
  bwdStepOptions.direction = Direction::Backward();

  // get the material collector and configure it
  auto& bwdStepMaterialInteractor =
      bwdStepOptions.actorList.template get<MaterialInteractor>();
  bwdStepMaterialInteractor.recordInteractions = true;
  bwdStepMaterialInteractor.multipleScattering = false;
  bwdStepMaterialInteractor.energyLoss = false;

  double bwdStepStepMaterialInX0 = 0.;
  double bwdStepStepMaterialInL0 = 0.;

  if (debugMode) {
    // check if the surfaces are free
    std::cout << ">>> Backward steps to be processed sequentially ..."
              << std::endl;
    for (auto& bwdStepsC : bwdMaterial.materialInteractions) {
      std::cout << "--> Surface with " << bwdStepsC.surface->geometryId()
                << std::endl;
    }
  }

  // move forward step by step through the surfaces
  sParameters = *fwdResult.endParameters;
  for (auto& bwdSteps : bwdMaterial.materialInteractions) {
    if (debugMode) {
      std::cout << ">>> Backward step : "
                << sParameters.referenceSurface().geometryId() << " --> "
                << bwdSteps.surface->geometryId() << std::endl;
    }
    // make a forward step
    const auto& bwdStep =
        prop.propagate(sParameters, (*bwdSteps.surface), bwdStepOptions)
            .value();

    auto& bwdStepMaterial =
        bwdStep.template get<typename MaterialInteractor::result_type>();
    bwdStepStepMaterialInX0 += bwdStepMaterial.materialInX0;
    bwdStepStepMaterialInL0 += bwdStepMaterial.materialInL0;

    if (bwdStep.endParameters.has_value()) {
      // make sure the parameters do not run out of scope
      stepParameters.push_back(*bwdStep.endParameters);
      sParameters = stepParameters.back();
    }
  }
  // final destination surface
  const Surface& dbSurface = start.referenceSurface();

  if (debugMode) {
    std::cout << ">>> Backward step : "
              << sParameters.referenceSurface().geometryId() << " --> "
              << dSurface.geometryId() << std::endl;
  }

  const auto& bwdStepFinal =
      prop.propagate(sParameters, dbSurface, bwdStepOptions).value();

  auto& bwdStepMaterial =
      bwdStepFinal.template get<typename MaterialInteractor::result_type>();
  bwdStepStepMaterialInX0 += bwdStepMaterial.materialInX0;
  bwdStepStepMaterialInL0 += bwdStepMaterial.materialInL0;

  // backward-backward step compatibility test
  CHECK_CLOSE_REL(bwdStepStepMaterialInX0, bwdStepMaterialInX0, 1e-3);
  CHECK_CLOSE_REL(bwdStepStepMaterialInL0, bwdStepMaterialInL0, 1e-3);

  // Test the material affects the covariance into the right direction
  // get the material collector and configure it
  auto& covfwdMaterialInteractor =
      fwdOptions.actorList.template get<MaterialInteractor>();
  covfwdMaterialInteractor.recordInteractions = false;
  covfwdMaterialInteractor.energyLoss = true;
  covfwdMaterialInteractor.multipleScattering = true;

  // forward material test
  const auto& covfwdResult = prop.propagate(start, fwdOptions).value();

  BOOST_CHECK_LE(
      start.covariance()->determinant(),
      covfwdResult.endParameters->covariance().value().determinant());
}

// This test case checks that no segmentation fault appears
// - this tests the collection of surfaces
BOOST_DATA_TEST_CASE(
    test_material_collector,
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

  double p = pT / sin(theta);
  double q = -1 + 2 * charge;

  // define start parameters
  BoundSquareMatrix cov;
  // take some major correlations (off-diagonals)
  // clang-format off
    cov <<
     10_mm, 0, 0.123, 0, 0.5, 0,
     0, 10_mm, 0, 0.162, 0, 0,
     0.123, 0, 0.1, 0, 0, 0,
     0, 0.162, 0, 0.1, 0, 0,
     0.5, 0, 0, 0, 1_e / 10_GeV, 0,
     0, 0, 0, 0, 0, 1_us;
  // clang-format on
  CurvilinearTrackParameters start(Vector4(0, 0, 0, 0), phi, theta, q / p, cov,
                                   ParticleHypothesis::pion());

  runTest(epropagator, start);
  runTest(slpropagator, start);
}

}  // namespace Acts::Test
