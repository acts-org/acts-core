// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/HomogeneousVolumeMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Propagator/ConstrainedStep.hpp"
#include "Acts/Propagator/detail/VolumeMaterialInteraction.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"

#include <memory>

namespace tt = boost::test_tools;
using namespace Acts::UnitLiterals;

namespace Acts {
namespace Test {

/// @brief Simplified stepper state
struct StepperState {
  Vector3 pos = Vector3::Zero(), dir = Vector3::Zero();
  ConstrainedStep stepSize;
  double t = 0, p = 0, q = 0;
  bool covTransport = false;
  Direction navDir = Direction::Forward;
};

/// @brief Simplified navigator
struct NaivgatorState {
  TrackingVolume* currentVolume = nullptr;
};

/// @brief Simplified propgator state
struct State {
  struct {
    double mass = 0;
    int absPdgCode = 0;
  } options;

  StepperState stepping;
  NaivgatorState navigation;
};

/// @brief Simplified stepper
struct Stepper {
  Stepper() = default;

  Vector3 position(const StepperState& state) const { return state.pos; }

  double time(const StepperState& state) const { return state.t; }

  Vector3 direction(const StepperState& state) const { return state.dir; }

  double qOverP(const StepperState& state) const { return state.q / state.p; }

  double absoluteMomentum(const StepperState& state) const { return state.p; }

  double charge(const StepperState& state) const { return state.q; };
};

/// @brief Simplified navigator
struct Navigator {
  const TrackingVolume* currentVolume(const NaivgatorState& state) const {
    return state.currentVolume;
  }
};

BOOST_AUTO_TEST_CASE(volume_material_interaction_test) {
  // Create a Tracking Volume
  auto htrans = Transform3(Translation3{-10., -10., 0.});
  auto bound = std::make_shared<const CuboidVolumeBounds>(1_m, 1_m, 1_m);
  auto mat = makeSilicon();
  auto volMat = std::make_shared<const HomogeneousVolumeMaterial>(mat);
  auto volume = TrackingVolume::create(htrans, bound, volMat);

  // Create a propagator state
  State state;
  state.stepping.pos = Vector3(1., 2., 3.);
  state.stepping.dir = Vector3(4., 5., 6.);
  state.stepping.stepSize = ConstrainedStep(1);
  state.stepping.t = 7.;
  state.stepping.p = 8.;
  state.stepping.q = 9.;
  state.stepping.covTransport = true;
  state.stepping.navDir = Direction::Backward;
  state.options.mass = 10.;
  state.options.absPdgCode = 11;
  state.navigation.currentVolume = volume.get();

  Stepper stepper;
  Navigator navigator;

  // Build the VolumeMaterialInteraction & test assignments
  detail::VolumeMaterialInteraction volMatInt(volume.get(), state, stepper);
  BOOST_CHECK_EQUAL(volMatInt.volume, volume.get());
  BOOST_CHECK_EQUAL(volMatInt.pos, stepper.position(state.stepping));
  BOOST_CHECK_EQUAL(volMatInt.time, stepper.time(state.stepping));
  BOOST_CHECK_EQUAL(volMatInt.dir, stepper.direction(state.stepping));
  BOOST_CHECK_EQUAL(volMatInt.momentum,
                    stepper.absoluteMomentum(state.stepping));
  BOOST_CHECK_EQUAL(volMatInt.absQ, std::abs(stepper.charge(state.stepping)));
  CHECK_CLOSE_ABS(volMatInt.qOverP, stepper.qOverP(state.stepping), 1e-6);
  BOOST_CHECK_EQUAL(volMatInt.mass, state.options.mass);
  BOOST_CHECK_EQUAL(volMatInt.pdg, state.options.absPdgCode);
  BOOST_CHECK_EQUAL(volMatInt.performCovarianceTransport,
                    state.stepping.covTransport);
  BOOST_CHECK_EQUAL(volMatInt.navDir, state.stepping.navDir);

  // Evaluate the material
  bool result = volMatInt.evaluateMaterialSlab(state, navigator);
  BOOST_CHECK(result);
  BOOST_CHECK_EQUAL(volMatInt.slab.material(), mat);
  BOOST_CHECK_EQUAL(volMatInt.slab.thickness(),
                    state.stepping.stepSize.value());
  BOOST_CHECK_EQUAL(volMatInt.pathCorrection, 0.);

  // Evaluate the material without a tracking volume
  state.navigation.currentVolume = nullptr;
  result = volMatInt.evaluateMaterialSlab(state, navigator);
  BOOST_CHECK(!result);
  BOOST_CHECK_EQUAL(volMatInt.pathCorrection, 0.);
}

}  // namespace Test
}  // namespace Acts
