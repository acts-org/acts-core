// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GridPortalLink.hpp"
#include "Acts/Geometry/Portal.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/TrapezoidPortalShell.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Geometry/TrivialPortalLink.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"

using namespace Acts::UnitLiterals;

namespace Acts::Test {
GeometryContext gctx;

std::size_t getVolumeIndex() {
  static std::size_t i = 1;
  return i++;
}

auto makeVolume(auto&&... pars) {
  TrackingVolume vol(Transform3::Identity(),
                     std::make_shared<TrapezoidVolumeBounds>(
                         std::forward<decltype(pars)>(pars)...));
  vol.setVolumeName("trapezoid" + std::to_string(getVolumeIndex()));
  return vol;
};

auto logger = Acts::getDefaultLogger("UnitTests", Acts::Logging::VERBOSE);

BOOST_AUTO_TEST_SUITE(TrapezoidPortalShellTests)

BOOST_AUTO_TEST_CASE(ConstructionFromVolume) {
  // construct a trapezoid tracking volume from which we are gonna build the
  // shell
  auto trapVol = makeVolume(20_cm, 10_cm, 15_cm, 15_cm);

  // check if indeed invalid for no trapezoid volume

  TrackingVolume cylVolume(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(10_mm, 20_mm, 10_mm));

  BOOST_CHECK_THROW(SingleTrapezoidPortalShell{cylVolume},
                    std::invalid_argument);

  SingleTrapezoidPortalShell trapShell{trapVol};

  // check if the shell is valid and has the expected number of portals
  BOOST_CHECK(trapShell.isValid());
  BOOST_CHECK_EQUAL(trapShell.size(), 6);

  // check if the protals are consistent with their orientation in the shell and
  // find the linked volume
  using enum TrapezoidVolumeBounds::Face;

  // check XY face in negative Z
  const auto* nXY = trapShell.portal(NegativeZFaceXY);

  BOOST_REQUIRE_NE(nXY, nullptr);
  BOOST_CHECK_EQUAL(
      nXY->resolveVolume(gctx, Vector3{5_cm, 10_cm, -15_cm}, Vector3::UnitZ())
          .value(),
      &trapVol);
  BOOST_CHECK_EQUAL(
      nXY->resolveVolume(gctx, Vector3{5_cm, 10_cm, -15_cm}, -Vector3::UnitZ())
          .value(),
      nullptr);

  // check XY face in positive Z
  const auto* pXY = trapShell.portal(PositiveZFaceXY);

  BOOST_REQUIRE_NE(pXY, nullptr);

  BOOST_CHECK_EQUAL(
      pXY->resolveVolume(gctx, Vector3{5_cm, 10_cm, 15_cm}, -Vector3::UnitZ())
          .value(),
      &trapVol);
  BOOST_CHECK_EQUAL(
      pXY->resolveVolume(gctx, Vector3{5_cm, 10_cm, 15_cm}, Vector3::UnitZ())
          .value(),
      nullptr);

  // check the trapezoidAlpha face placed in negative X
  const auto* nYZ = trapShell.portal(TrapezoidFaceAlpha);
  BOOST_REQUIRE_NE(nYZ, nullptr);
  BOOST_CHECK_EQUAL(
      nYZ->resolveVolume(gctx, Vector3{-15_cm, 0_cm, 0_cm}, Vector3::UnitX())
          .value(),
      &trapVol);
  BOOST_CHECK_EQUAL(
      nYZ->resolveVolume(gctx, Vector3{-15_cm, 0_cm, 0_cm}, -Vector3::UnitX())
          .value(),
      nullptr);

  // check the trapezoidBeta face placed in positive x
  const auto* pYZ = trapShell.portal(TrapezoidFaceBeta);
  BOOST_REQUIRE_NE(pYZ, nullptr);
  BOOST_CHECK_EQUAL(
      pYZ->resolveVolume(gctx, Vector3{15_cm, 0_cm, 0_cm}, -Vector3::UnitX())
          .value(),
      &trapVol);
  BOOST_CHECK_EQUAL(
      pYZ->resolveVolume(gctx, Vector3{15_cm, 0_cm, 0_cm}, Vector3::UnitX())
          .value(),
      nullptr);

  // check the ZX face in negative y
  const auto* nZX = trapShell.portal(NegativeYFaceZX);
  BOOST_CHECK_NE(nZX, nullptr);
  BOOST_CHECK_EQUAL(
      nZX->resolveVolume(gctx, Vector3{0_cm, -15_cm, 0_cm}, Vector3::UnitY())
          .value(),
      &trapVol);
  BOOST_CHECK_EQUAL(
      nZX->resolveVolume(gctx, Vector3{0_cm, -15_cm, 0_cm}, -Vector3::UnitY())
          .value(),
      nullptr);

  // check the ZX face in positive y
  const auto* pZX = trapShell.portal(PositiveYFaceZX);
  BOOST_CHECK_NE(pZX, nullptr);
  BOOST_CHECK_EQUAL(
      pZX->resolveVolume(gctx, Vector3{0_cm, 15_cm, 0_cm}, -Vector3::UnitY())
          .value(),
      &trapVol);
  BOOST_CHECK_EQUAL(
      pZX->resolveVolume(gctx, Vector3{0_cm, 15_cm, 0_cm}, Vector3::UnitY())
          .value(),
      nullptr);
}

BOOST_AUTO_TEST_CASE(PortalAssignment) {
  using enum TrapezoidVolumeBounds::Face;

  // make a trapezoid volume
  auto trapVol = makeVolume(5_cm, 8_cm, 10_cm, 15_cm);

  SingleTrapezoidPortalShell trapShell{trapVol};

  // get the portal faces
  const auto* nXY = trapShell.portal(NegativeZFaceXY);
  const auto* pXY = trapShell.portal(PositiveZFaceXY);

  const auto* nYZ = trapShell.portal(TrapezoidFaceAlpha);
  const auto* pYZ = trapShell.portal(TrapezoidFaceBeta);

  auto* nZX = trapShell.portal(NegativeYFaceZX);
  auto* pZX = trapShell.portal(PositiveYFaceZX);

  // setting new portals for NegativeYFaceZX and PositiveYFaceZX faces
  BOOST_REQUIRE_NE(nZX, nullptr);
  BOOST_REQUIRE_NE(pZX, nullptr);

  auto* pZXLink = dynamic_cast<const TrivialPortalLink*>(
      pZX->getLink(Direction::OppositeNormal()));
  BOOST_REQUIRE_NE(pZXLink, nullptr);

  auto grid = pZXLink->makeGrid(AxisDirection::AxisY);

  auto newPortalPZX =
      std::make_shared<Portal>(Direction::OppositeNormal(), std::move(grid));
  trapShell.setPortal(newPortalPZX, PositiveYFaceZX);
  BOOST_CHECK_EQUAL(trapShell.portal(PositiveYFaceZX), newPortalPZX.get());

  auto* nZXLink = dynamic_cast<const TrivialPortalLink*>(
      nZX->getLink(Direction::AlongNormal()));
  BOOST_REQUIRE_NE(nZXLink, nullptr);

  grid = nZXLink->makeGrid(AxisDirection::AxisY);

  auto newPortalNZX =
      std::make_shared<Portal>(Direction::AlongNormal(), std::move(grid));
  trapShell.setPortal(newPortalNZX, NegativeYFaceZX);
  BOOST_CHECK_EQUAL(trapShell.portal(NegativeYFaceZX), newPortalNZX.get());

  // check the other portals
  BOOST_CHECK_EQUAL(trapShell.portal(NegativeZFaceXY), nXY);
  BOOST_CHECK_EQUAL(trapShell.portal(PositiveZFaceXY), pXY);
  BOOST_CHECK_EQUAL(trapShell.portal(TrapezoidFaceAlpha), nYZ);
  BOOST_CHECK_EQUAL(trapShell.portal(TrapezoidFaceBeta), pYZ);
}

BOOST_AUTO_TEST_CASE(Fill) {
  using enum TrapezoidVolumeBounds::Face;

  auto trapVol = makeVolume(5_cm, 10_cm, 10_cm, 10_cm);

  SingleTrapezoidPortalShell trapShell{trapVol};

  // without filling the shell with the volume the portal link to this direction
  // should not exist
  BOOST_CHECK_EQUAL(
      trapShell.portal(NegativeZFaceXY)->getLink(Direction::OppositeNormal()),
      nullptr);

  // create a volume nexto to the previous one and fill the shell with the new
  // volume to fill the available slots of the shell
  TrackingVolume trapVol2(
      Transform3::Identity() * Translation3(Vector3::UnitZ() * 15_cm),
      std::make_shared<TrapezoidVolumeBounds>(5_cm, 10_cm, 10_cm, 5_cm),
      "trapVol2");

  trapShell.fill(trapVol2);

  BOOST_CHECK_NE(
      trapShell.portal(NegativeZFaceXY)->getLink(Direction::OppositeNormal()),
      nullptr);
}

BOOST_AUTO_TEST_CASE(ApplyToVolume) {
  using enum TrapezoidVolumeBounds::Face;

  auto trapVol = makeVolume(5_cm, 10_cm, 10_cm, 10_cm);

  SingleTrapezoidPortalShell trapShell{trapVol};

  // volume has not portals assigned yet
  BOOST_CHECK_EQUAL(trapVol.portals().size(), 0);

  trapShell.applyToVolume();

  // now volume should have its portals
  BOOST_CHECK_EQUAL(trapVol.portals().size(), 6);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
