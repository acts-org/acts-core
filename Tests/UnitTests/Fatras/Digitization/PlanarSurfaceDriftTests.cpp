// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "ActsFatras/Digitization/PlanarSurfaceDrift.hpp"

#include <array>
#include <memory>

namespace ActsFatras {

BOOST_AUTO_TEST_SUITE(Digitization)

BOOST_AUTO_TEST_CASE(PlanarSurfaceDrift) {
  Acts::GeometryContext geoCtx;

  ActsFatras::PlanarSurfaceDrift psd;

  Acts::Vector3 cPosition = Acts::Vector3(10., 50., 12.);
  Acts::Vector3 cNormal = Acts::Vector3(1., 1., 1.).normalized();

  auto planeSurface =
      Acts::CurvilinearSurface(cPosition, cNormal).planeSurface();

  double depletion = 0.250;

  // Nominal intersection
  Acts::Vector3 noDrift(0., 0., 0.);
  Acts::Vector3 holeDrift = Acts::Vector3(0.5, 0., 1.).normalized();
  Acts::Vector3 chargeDrift = Acts::Vector3(0.5, 0., -1.).normalized();

  // Intersect surface at normal direction and no drift
  //
  // -> resulting segment must have entry & exit at (0,0) local coordinates
  auto noDriftSegment = psd.toReadout(geoCtx, *planeSurface, depletion,
                                      cPosition, cNormal, noDrift);

  CHECK_CLOSE_ABS(noDriftSegment[0].x(), 0., Acts::s_epsilon);
  CHECK_CLOSE_ABS(noDriftSegment[0].y(), 0., Acts::s_epsilon);
  CHECK_CLOSE_ABS(noDriftSegment[1].x(), 0., Acts::s_epsilon);
  CHECK_CLOSE_ABS(noDriftSegment[1].y(), 0., Acts::s_epsilon);

  Acts::Vector3 particleDir = Acts::Vector3(2., 1., 1.).normalized();

  // Intersect surface at particleDirection != normal and no drift
  //
  // -> local segment must be symmetric around (0,0)
  noDriftSegment = psd.toReadout(geoCtx, *planeSurface, depletion, cPosition,
                                 particleDir, noDrift);

  CHECK_CLOSE_ABS(noDriftSegment[0].x(), -noDriftSegment[1].x(),
                  Acts::s_epsilon);
  CHECK_CLOSE_ABS(noDriftSegment[0].y(), -noDriftSegment[1].y(),
                  Acts::s_epsilon);

  // Intersect surface at particleDirection != normal and a drift somewhat along
  // the normal and x
  //
  // -> local segment must not be symmetric around (0,0)
  // -> segment exit at pos local z remains unchanged
  // -> segment entry at neg local z changes in x, remains unchanged in y
  auto driftedSegment = psd.toReadout(geoCtx, *planeSurface, depletion,
                                      cPosition, particleDir, holeDrift);

  BOOST_CHECK(std::abs(driftedSegment[0].x() - driftedSegment[1].x()) >
              Acts::s_epsilon);
  BOOST_CHECK(std::abs(driftedSegment[0].y() - driftedSegment[1].y()) >
              Acts::s_epsilon);
  CHECK_CLOSE_ABS(noDriftSegment[1].x(), driftedSegment[1].x(),
                  Acts::s_epsilon);
  CHECK_CLOSE_ABS(noDriftSegment[1].y(), driftedSegment[1].y(),
                  Acts::s_epsilon);
  BOOST_CHECK(std::abs(driftedSegment[0].x() - noDriftSegment[0].x()) >
              Acts::s_epsilon);
  CHECK_CLOSE_ABS(driftedSegment[0].y(), noDriftSegment[0].y(),
                  Acts::s_epsilon);

  // Intersect surface at particleDirection != normal and a drift somewhat
  // opposite the normal and y
  //
  // -> local segment must not be symmetric around (0,0)
  // -> segment entry at neg local z remains unchanged
  // -> segment exit at pos local z changes in x, remains unchanged in y
  driftedSegment = psd.toReadout(geoCtx, *planeSurface, depletion, cPosition,
                                 particleDir, chargeDrift);

  BOOST_CHECK(std::abs(driftedSegment[0].x() - driftedSegment[1].x()) >
              Acts::s_epsilon);
  BOOST_CHECK(std::abs(driftedSegment[0].y() - driftedSegment[1].y()) >
              Acts::s_epsilon);
  CHECK_CLOSE_ABS(noDriftSegment[0].x(), driftedSegment[0].x(),
                  Acts::s_epsilon);
  CHECK_CLOSE_ABS(noDriftSegment[0].y(), driftedSegment[0].y(),
                  Acts::s_epsilon);
  BOOST_CHECK(std::abs(driftedSegment[1].x() - noDriftSegment[1].x()) >
              Acts::s_epsilon);
  CHECK_CLOSE_ABS(driftedSegment[1].y(), noDriftSegment[1].y(),
                  Acts::s_epsilon);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace ActsFatras
