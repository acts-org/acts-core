// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/detail/SteppingLogger.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Tests/CommonHelpers/CubicTrackingGeometry.hpp"
#include "Acts/Utilities/Logger.hpp"

using namespace Acts::UnitLiterals;

/// This tests intend to check the behaviour of the navigator in cases where the
/// straight-line approach for the layer resolval can fail. This is in
/// particular the case with bent tracks in telesocpe-like geometries, and can
/// be fixed by not doing the bounds check in the initial resolving.

using MagneticField = Acts::ConstantBField;
using Stepper = Acts::EigenStepper<>;
using Propagator = Acts::Propagator<Stepper, Acts::Navigator>;

const Acts::GeometryContext geoCtx;
const Acts::MagneticFieldContext magCtx;

std::vector<double> xPositionsOfPassedSurfaces(Acts::Navigator::Config navCfg,
                                               double bz) {
  auto magField = std::make_shared<MagneticField>(Acts::Vector3(0.0, 0.0, bz));
  Acts::Test::CubicTrackingGeometry cubicBuilder(geoCtx);

  navCfg.trackingGeometry = cubicBuilder();
  Stepper stepper(std::move(magField));
  Propagator propagator(
      stepper,
      Acts::Navigator(navCfg,
                      Acts::getDefaultLogger("nav", Acts::Logging::VERBOSE)),
      Acts::getDefaultLogger("nav", Acts::Logging::VERBOSE));

  // Start with a slightly tilted direction that does not hit the surfaces at
  // x=2000 with 0 B-Field
  Acts::Vector3 dir = Acts::Vector3{1.0_m, 0.3_m, 0.0_m};

  // Start a bit in the volume 2, so we do not have any boundary checking for
  // the volume transition in the log
  Acts::CurvilinearTrackParameters start(
      Acts::Vector4(0.01, 0, 0, 0), dir.normalized(), 1 / 1_GeV, std::nullopt,
      Acts::ParticleHypothesis::pion());

  Propagator::Options<
      Acts::ActorList<Acts::detail::SteppingLogger, Acts::EndOfWorldReached>>
      opts(geoCtx, magCtx);

  auto res = propagator.propagate(start, opts);

  BOOST_CHECK(res.ok());

  const auto &stepLog = res->get<Acts::detail::SteppingLogger::result_type>();

  std::vector<double> xPositions;
  for (const auto &step : stepLog.steps) {
    if (step.surface) {
      xPositions.push_back(step.surface->center(geoCtx)[Acts::ePos0]);
    }
  }

  return xPositions;
}

BOOST_AUTO_TEST_CASE(with_boundary_check_no_bfield) {
  auto navCfg = Acts::Navigator::Config{};
  const auto xPositions = xPositionsOfPassedSurfaces(navCfg, 0.0_T);

  // without bfield we exit at the side so we don't hit the surfaces at x ~
  // 2000 and also not the boundary surface at x = 3000, regardless of the
  // boundary checking
  BOOST_CHECK_EQUAL(std::count(xPositions.begin(), xPositions.end(), 999.0), 1);
  BOOST_CHECK_EQUAL(std::count(xPositions.begin(), xPositions.end(), 1001.0),
                    1);
  BOOST_CHECK_EQUAL(std::count(xPositions.begin(), xPositions.end(), 1999.0),
                    0);
  BOOST_CHECK_EQUAL(std::count(xPositions.begin(), xPositions.end(), 2001.0),
                    0);
  BOOST_CHECK_EQUAL(std::count(xPositions.begin(), xPositions.end(), 3000.0),
                    0);
}

BOOST_AUTO_TEST_CASE(with_boundary_check_with_bfield) {
  auto navCfg = Acts::Navigator::Config{};
  const auto xPositions = xPositionsOfPassedSurfaces(navCfg, 0.5_T);

  // With default navigation config we miss the surfaces at x ~ 2000, but hit
  // the boundary surface at x = 3000
  BOOST_CHECK_EQUAL(std::count(xPositions.begin(), xPositions.end(), 999.0), 1);
  BOOST_CHECK_EQUAL(std::count(xPositions.begin(), xPositions.end(), 1001.0),
                    1);
  BOOST_CHECK_EQUAL(std::count(xPositions.begin(), xPositions.end(), 1999.0),
                    0);
  BOOST_CHECK_EQUAL(std::count(xPositions.begin(), xPositions.end(), 2001.0),
                    0);
  BOOST_CHECK_EQUAL(std::count(xPositions.begin(), xPositions.end(), 3000.0),
                    1);
}
