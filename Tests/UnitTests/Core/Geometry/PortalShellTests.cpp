// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/tools/context.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GridPortalLink.hpp"
#include "Acts/Geometry/Portal.hpp"
#include "Acts/Geometry/PortalShell.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/TrivialPortalLink.hpp"
#include "Acts/Surfaces/SurfaceMergingException.hpp"

#include <stdexcept>

using namespace Acts::UnitLiterals;

namespace Acts::Test {
GeometryContext gctx;

std::size_t getVolumeIndex() {
  static std::size_t i = 1;
  return i++;
}

auto makeVolume(auto&&... pars) {
  TrackingVolume vol(Transform3::Identity(),
                     std::make_shared<CylinderVolumeBounds>(
                         std::forward<decltype(pars)>(pars)...));
  vol.setVolumeName("cyl" + std::to_string(getVolumeIndex()));
  return vol;
};

auto logger = Acts::getDefaultLogger("UnitTests", Acts::Logging::VERBOSE);

BOOST_AUTO_TEST_SUITE(PortalShellTests)

BOOST_AUTO_TEST_CASE(ConstructionFromVolume) {
  // - Cylinder
  // |           | no phi | phi |
  // | --------- | ------ | --- |
  // | rMin > 0  | 1      | 3   |
  // | rMin == 0 | 2      | 4   |

  auto cyl1 = makeVolume(30_mm, 40_mm, 100_mm);
  auto cyl2 = makeVolume(0_mm, 40_mm, 100_mm);
  auto cyl3 = makeVolume(30_mm, 40_mm, 100_mm, 45_degree);
  auto cyl4 = makeVolume(0_mm, 40_mm, 100_mm, 45_degree);

  TrackingVolume boxVolume(
      Transform3::Identity(),
      std::make_shared<CuboidVolumeBounds>(10_mm, 10_mm, 10_mm));

  BOOST_CHECK_THROW(SingleCylinderPortalShell{boxVolume},
                    std::invalid_argument);

  SingleCylinderPortalShell shell1{cyl1};
  BOOST_CHECK_EQUAL(shell1.size(), 4);

  using enum CylinderPortalShell::Face;

  const auto* pDisc = shell1.portal(PositiveDisc);
  BOOST_REQUIRE_NE(pDisc, nullptr);
  BOOST_CHECK_EQUAL(
      pDisc
          ->resolveVolume(gctx, Vector3{35_mm, 0_mm, 100_mm}, -Vector3::UnitZ())
          .value(),
      &cyl1);
  BOOST_CHECK_EQUAL(
      pDisc->resolveVolume(gctx, Vector3{35_mm, 0_mm, 100_mm}, Vector3::UnitZ())
          .value(),
      nullptr);

  const auto* nDisc = shell1.portal(NegativeDisc);
  BOOST_REQUIRE_NE(nDisc, nullptr);
  BOOST_CHECK_EQUAL(nDisc
                        ->resolveVolume(gctx, Vector3{35_mm, 0_mm, -100_mm},
                                        -Vector3::UnitZ())
                        .value(),
                    nullptr);
  BOOST_CHECK_EQUAL(
      nDisc
          ->resolveVolume(gctx, Vector3{35_mm, 0_mm, -100_mm}, Vector3::UnitZ())
          .value(),
      &cyl1);

  const auto* oCyl = shell1.portal(OuterCylinder);
  BOOST_REQUIRE_NE(oCyl, nullptr);
  BOOST_CHECK_EQUAL(
      oCyl->resolveVolume(gctx, Vector3{40_mm, 0_mm, 10_mm}, Vector3::UnitX())
          .value(),
      nullptr);
  BOOST_CHECK_EQUAL(
      oCyl->resolveVolume(gctx, Vector3{40_mm, 0_mm, 10_mm}, -Vector3::UnitX())
          .value(),
      &cyl1);

  const auto* iCyl = shell1.portal(InnerCylinder);
  BOOST_REQUIRE_NE(iCyl, nullptr);
  BOOST_CHECK_EQUAL(
      iCyl->resolveVolume(gctx, Vector3{30_mm, 0_mm, 10_mm}, Vector3::UnitX())
          .value(),
      &cyl1);
  BOOST_CHECK_EQUAL(
      iCyl->resolveVolume(gctx, Vector3{30_mm, 0_mm, 10_mm}, -Vector3::UnitX())
          .value(),
      nullptr);

  SingleCylinderPortalShell shell2{cyl2};
  BOOST_CHECK_EQUAL(shell2.size(), 3);

  pDisc = shell2.portal(PositiveDisc);
  BOOST_REQUIRE_NE(pDisc, nullptr);
  BOOST_CHECK_EQUAL(
      pDisc
          ->resolveVolume(gctx, Vector3{35_mm, 0_mm, 100_mm}, -Vector3::UnitZ())
          .value(),
      &cyl2);
  BOOST_CHECK_EQUAL(
      pDisc->resolveVolume(gctx, Vector3{35_mm, 0_mm, 100_mm}, Vector3::UnitZ())
          .value(),
      nullptr);

  nDisc = shell2.portal(NegativeDisc);
  BOOST_REQUIRE_NE(nDisc, nullptr);
  BOOST_CHECK_EQUAL(nDisc
                        ->resolveVolume(gctx, Vector3{35_mm, 0_mm, -100_mm},
                                        -Vector3::UnitZ())
                        .value(),
                    nullptr);
  BOOST_CHECK_EQUAL(
      nDisc
          ->resolveVolume(gctx, Vector3{35_mm, 0_mm, -100_mm}, Vector3::UnitZ())
          .value(),
      &cyl2);

  oCyl = shell2.portal(OuterCylinder);
  BOOST_REQUIRE_NE(oCyl, nullptr);
  BOOST_CHECK_EQUAL(
      oCyl->resolveVolume(gctx, Vector3{40_mm, 0_mm, 10_mm}, Vector3::UnitX())
          .value(),
      nullptr);
  BOOST_CHECK_EQUAL(
      oCyl->resolveVolume(gctx, Vector3{40_mm, 0_mm, 10_mm}, -Vector3::UnitX())
          .value(),
      &cyl2);

  iCyl = shell2.portal(InnerCylinder);
  BOOST_CHECK_EQUAL(iCyl, nullptr);

  SingleCylinderPortalShell shell3{cyl3};
  BOOST_CHECK_EQUAL(shell3.size(), 6);

  pDisc = shell3.portal(PositiveDisc);
  BOOST_REQUIRE_NE(pDisc, nullptr);
  BOOST_CHECK_EQUAL(
      pDisc
          ->resolveVolume(gctx, Vector3{35_mm, 0_mm, 100_mm}, -Vector3::UnitZ())
          .value(),
      &cyl3);
  BOOST_CHECK_EQUAL(
      pDisc->resolveVolume(gctx, Vector3{35_mm, 0_mm, 100_mm}, Vector3::UnitZ())
          .value(),
      nullptr);

  nDisc = shell3.portal(NegativeDisc);
  BOOST_REQUIRE_NE(nDisc, nullptr);
  BOOST_CHECK_EQUAL(nDisc
                        ->resolveVolume(gctx, Vector3{35_mm, 0_mm, -100_mm},
                                        -Vector3::UnitZ())
                        .value(),
                    nullptr);
  BOOST_CHECK_EQUAL(
      nDisc
          ->resolveVolume(gctx, Vector3{35_mm, 0_mm, -100_mm}, Vector3::UnitZ())
          .value(),
      &cyl3);

  oCyl = shell3.portal(OuterCylinder);
  BOOST_REQUIRE_NE(oCyl, nullptr);
  BOOST_CHECK_EQUAL(
      oCyl->resolveVolume(gctx, Vector3{40_mm, 0_mm, 10_mm}, Vector3::UnitX())
          .value(),
      nullptr);
  BOOST_CHECK_EQUAL(
      oCyl->resolveVolume(gctx, Vector3{40_mm, 0_mm, 10_mm}, -Vector3::UnitX())
          .value(),
      &cyl3);

  iCyl = shell3.portal(InnerCylinder);
  BOOST_REQUIRE_NE(iCyl, nullptr);
  BOOST_CHECK_EQUAL(
      iCyl->resolveVolume(gctx, Vector3{30_mm, 0_mm, 10_mm}, Vector3::UnitX())
          .value(),
      &cyl3);
  BOOST_CHECK_EQUAL(
      iCyl->resolveVolume(gctx, Vector3{30_mm, 0_mm, 10_mm}, -Vector3::UnitX())
          .value(),
      nullptr);

  auto anglePoint = [](double angle, double r, double z) {
    return Vector3{r * std::cos(angle), r * std::sin(angle), z};
  };

  const auto* nPhi = shell3.portal(NegativePhiPlane);
  BOOST_REQUIRE_NE(nPhi, nullptr);
  Vector3 point = anglePoint(-45_degree, 35_mm, 10_mm);
  Vector3 dir = Vector3::UnitZ().cross(point).normalized();
  Vector3 idir = (-Vector3::UnitZ()).cross(point).normalized();
  BOOST_CHECK_EQUAL(nPhi->resolveVolume(gctx, point, dir).value(), nullptr);
  BOOST_CHECK_EQUAL(nPhi->resolveVolume(gctx, point, idir).value(), &cyl3);

  const auto* pPhi = shell3.portal(PositivePhiPlane);
  BOOST_REQUIRE_NE(pPhi, nullptr);
  point = anglePoint(45_degree, 35_mm, 10_mm);
  dir = Vector3::UnitZ().cross(point).normalized();
  idir = (-Vector3::UnitZ()).cross(point).normalized();
  BOOST_CHECK_EQUAL(pPhi->resolveVolume(gctx, point, dir).value(), nullptr);
  BOOST_CHECK_EQUAL(pPhi->resolveVolume(gctx, point, idir).value(), &cyl3);

  SingleCylinderPortalShell shell4{cyl4};
  BOOST_CHECK_EQUAL(shell4.size(), 5);

  pDisc = shell4.portal(PositiveDisc);
  BOOST_REQUIRE_NE(pDisc, nullptr);
  BOOST_CHECK_EQUAL(
      pDisc
          ->resolveVolume(gctx, Vector3{35_mm, 0_mm, 100_mm}, -Vector3::UnitZ())
          .value(),
      &cyl4);
  BOOST_CHECK_EQUAL(
      pDisc->resolveVolume(gctx, Vector3{35_mm, 0_mm, 100_mm}, Vector3::UnitZ())
          .value(),
      nullptr);

  nDisc = shell4.portal(NegativeDisc);
  BOOST_REQUIRE_NE(nDisc, nullptr);
  BOOST_CHECK_EQUAL(nDisc
                        ->resolveVolume(gctx, Vector3{35_mm, 0_mm, -100_mm},
                                        -Vector3::UnitZ())
                        .value(),
                    nullptr);
  BOOST_CHECK_EQUAL(
      nDisc
          ->resolveVolume(gctx, Vector3{35_mm, 0_mm, -100_mm}, Vector3::UnitZ())
          .value(),
      &cyl4);

  oCyl = shell4.portal(OuterCylinder);
  BOOST_REQUIRE_NE(oCyl, nullptr);
  BOOST_CHECK_EQUAL(
      oCyl->resolveVolume(gctx, Vector3{40_mm, 0_mm, 10_mm}, Vector3::UnitX())
          .value(),
      nullptr);
  BOOST_CHECK_EQUAL(
      oCyl->resolveVolume(gctx, Vector3{40_mm, 0_mm, 10_mm}, -Vector3::UnitX())
          .value(),
      &cyl4);

  iCyl = shell4.portal(InnerCylinder);
  BOOST_CHECK_EQUAL(iCyl, nullptr);

  nPhi = shell4.portal(NegativePhiPlane);
  BOOST_REQUIRE_NE(nPhi, nullptr);
  point = anglePoint(-45_degree, 35_mm, 10_mm);
  dir = Vector3::UnitZ().cross(point).normalized();
  idir = (-Vector3::UnitZ()).cross(point).normalized();
  BOOST_CHECK_EQUAL(nPhi->resolveVolume(gctx, point, dir).value(), nullptr);
  BOOST_CHECK_EQUAL(nPhi->resolveVolume(gctx, point, idir).value(), &cyl4);

  pPhi = shell4.portal(PositivePhiPlane);
  BOOST_REQUIRE_NE(pPhi, nullptr);
  point = anglePoint(45_degree, 35_mm, 10_mm);
  dir = Vector3::UnitZ().cross(point).normalized();
  idir = (-Vector3::UnitZ()).cross(point).normalized();
  BOOST_CHECK_EQUAL(pPhi->resolveVolume(gctx, point, dir).value(), nullptr);
  BOOST_CHECK_EQUAL(pPhi->resolveVolume(gctx, point, idir).value(), &cyl4);
}

//              outer cylinder
//          +----------+----------+
//          |          |          |
// negative |          v          | positive
//     disc +-->               <--+ disc
//          |          ^          |
//          |          |          |
//          +----------+----------+
//              inner cylinder

BOOST_AUTO_TEST_CASE(PortalAssignment) {
  using enum CylinderPortalShell::Face;
  TrackingVolume vol(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 100_mm, 100_mm));

  SingleCylinderPortalShell shell{vol};

  const auto* iCyl = shell.portal(InnerCylinder);
  const auto* pDisc = shell.portal(PositiveDisc);
  auto* oCyl = shell.portal(OuterCylinder);
  auto* nDisc = shell.portal(NegativeDisc);

  // Setting new outer cylinder
  BOOST_REQUIRE_NE(oCyl, nullptr);
  auto* oCylLink = dynamic_cast<const TrivialPortalLink*>(
      oCyl->getLink(Direction::OppositeNormal));
  BOOST_REQUIRE_NE(oCylLink, nullptr);

  auto grid = oCylLink->makeGrid(BinningValue::binZ);

  auto portal2 =
      std::make_shared<Portal>(Direction::OppositeNormal, std::move(grid));
  shell.setPortal(portal2, OuterCylinder);
  BOOST_CHECK_EQUAL(shell.portal(OuterCylinder), portal2.get());

  // Other portals should stay the same
  BOOST_CHECK_EQUAL(shell.portal(InnerCylinder), iCyl);
  BOOST_CHECK_EQUAL(shell.portal(PositiveDisc), pDisc);
  BOOST_CHECK_EQUAL(shell.portal(NegativeDisc), nDisc);

  // Setting new negative disc
  BOOST_REQUIRE_NE(nDisc, nullptr);
  auto* nDiscLink = dynamic_cast<const TrivialPortalLink*>(
      nDisc->getLink(Direction::AlongNormal));
  BOOST_REQUIRE_NE(nDiscLink, nullptr);

  grid = nDiscLink->makeGrid(BinningValue::binR);

  auto portal3 =
      std::make_shared<Portal>(Direction::AlongNormal, std::move(grid));
  shell.setPortal(portal3, NegativeDisc);
  BOOST_CHECK_EQUAL(shell.portal(NegativeDisc), portal3.get());

  // Other portals should stay the same
  BOOST_CHECK_EQUAL(shell.portal(OuterCylinder), portal2.get());
  BOOST_CHECK_EQUAL(shell.portal(InnerCylinder), iCyl);
  BOOST_CHECK_EQUAL(shell.portal(PositiveDisc), pDisc);
}

BOOST_AUTO_TEST_SUITE(CylinderStack)
BOOST_AUTO_TEST_CASE(ZDirection) {
  using enum CylinderPortalShell::Face;
  BOOST_TEST_CONTEXT("rMin>0") {
    TrackingVolume vol1(
        Transform3{Translation3{Vector3::UnitZ() * -100_mm}},
        std::make_shared<CylinderVolumeBounds>(30_mm, 100_mm, 100_mm));

    TrackingVolume vol2(
        Transform3{Translation3{Vector3::UnitZ() * 100_mm}},
        std::make_shared<CylinderVolumeBounds>(30_mm, 100_mm, 100_mm));

    SingleCylinderPortalShell shell1{vol1};
    SingleCylinderPortalShell shell2{vol2};

    BOOST_CHECK_NE(shell1.portal(PositiveDisc), shell2.portal(NegativeDisc));

    CylinderStackPortalShell stack{
        gctx, {&shell1, &shell2}, BinningValue::binZ};
    BOOST_CHECK_EQUAL(stack.size(), 4);

    const auto* iCyl = stack.portal(InnerCylinder);
    BOOST_CHECK_EQUAL(shell1.portal(InnerCylinder), iCyl);
    BOOST_CHECK_EQUAL(shell2.portal(InnerCylinder), iCyl);

    const auto* oCyl = stack.portal(OuterCylinder);
    BOOST_CHECK_EQUAL(shell1.portal(OuterCylinder), oCyl);
    BOOST_CHECK_EQUAL(shell2.portal(OuterCylinder), oCyl);

    BOOST_CHECK_EQUAL(stack.portal(PositiveDisc), shell2.portal(PositiveDisc));
    BOOST_CHECK_EQUAL(stack.portal(NegativeDisc), shell1.portal(NegativeDisc));

    BOOST_CHECK_EQUAL(stack.portal(NegativePhiPlane), nullptr);
    BOOST_CHECK_EQUAL(stack.portalPtr(NegativePhiPlane), nullptr);

    BOOST_CHECK_EQUAL(stack.portal(PositivePhiPlane), nullptr);
    BOOST_CHECK_EQUAL(stack.portalPtr(PositivePhiPlane), nullptr);

    // Disc portals have been fused
    BOOST_CHECK_EQUAL(shell1.portal(PositiveDisc), shell2.portal(NegativeDisc));

    shell1 = SingleCylinderPortalShell{vol1};
    shell2 = SingleCylinderPortalShell{vol2};

    BOOST_CHECK_THROW(
        CylinderStackPortalShell(gctx, {&shell1, &shell2}, BinningValue::binR),
        SurfaceMergingException);
  }

  BOOST_TEST_CONTEXT("rMin==0") {
    TrackingVolume vol1(
        Transform3{Translation3{Vector3::UnitZ() * -100_mm}},
        std::make_shared<CylinderVolumeBounds>(0_mm, 100_mm, 100_mm));

    TrackingVolume vol2(
        Transform3{Translation3{Vector3::UnitZ() * 100_mm}},
        std::make_shared<CylinderVolumeBounds>(0_mm, 100_mm, 100_mm));

    SingleCylinderPortalShell shell1{vol1};
    SingleCylinderPortalShell shell2{vol2};

    BOOST_CHECK_EQUAL(shell1.portal(InnerCylinder), nullptr);
    BOOST_CHECK_EQUAL(shell2.portal(InnerCylinder), nullptr);

    BOOST_CHECK_NE(shell1.portal(PositiveDisc), shell2.portal(NegativeDisc));

    CylinderStackPortalShell stack{
        gctx, {&shell1, &shell2}, BinningValue::binZ};
    BOOST_CHECK_EQUAL(stack.size(), 3);

    // Disc portals have been fused
    BOOST_CHECK_EQUAL(shell1.portal(PositiveDisc), shell2.portal(NegativeDisc));

    const auto* iCyl = stack.portal(InnerCylinder);
    BOOST_CHECK_EQUAL(iCyl, nullptr);

    const auto* oCyl = stack.portal(OuterCylinder);
    BOOST_CHECK_EQUAL(shell1.portal(OuterCylinder), oCyl);
    BOOST_CHECK_EQUAL(shell2.portal(OuterCylinder), oCyl);

    BOOST_CHECK_EQUAL(stack.portal(PositiveDisc), shell2.portal(PositiveDisc));
    BOOST_CHECK_EQUAL(stack.portal(NegativeDisc), shell1.portal(NegativeDisc));

    BOOST_CHECK_EQUAL(stack.portal(NegativePhiPlane), nullptr);
    BOOST_CHECK_EQUAL(stack.portalPtr(NegativePhiPlane), nullptr);

    BOOST_CHECK_EQUAL(stack.portal(PositivePhiPlane), nullptr);
    BOOST_CHECK_EQUAL(stack.portalPtr(PositivePhiPlane), nullptr);

    shell1 = SingleCylinderPortalShell{vol1};
    shell2 = SingleCylinderPortalShell{vol2};

    BOOST_CHECK_THROW(
        CylinderStackPortalShell(gctx, {&shell1, &shell2}, BinningValue::binR),
        SurfaceMergingException);
  }
}

BOOST_AUTO_TEST_CASE(RDirection) {
  using enum CylinderPortalShell::Face;
  BOOST_TEST_CONTEXT("rMin>0") {
    TrackingVolume vol1(
        Transform3::Identity(),
        std::make_shared<CylinderVolumeBounds>(30_mm, 100_mm, 100_mm));

    TrackingVolume vol2(
        Transform3::Identity(),
        std::make_shared<CylinderVolumeBounds>(100_mm, 150_mm, 100_mm));

    SingleCylinderPortalShell shell1{vol1};
    SingleCylinderPortalShell shell2{vol2};

    BOOST_CHECK_NE(shell1.portal(OuterCylinder), shell2.portal(InnerCylinder));

    CylinderStackPortalShell stack{
        gctx, {&shell1, &shell2}, BinningValue::binR};
    BOOST_CHECK_EQUAL(stack.size(), 4);

    // Internal cylinder portals have been fused
    BOOST_CHECK_EQUAL(shell1.portal(OuterCylinder),
                      shell2.portal(InnerCylinder));

    const auto* nDisc = stack.portal(NegativeDisc);
    const auto* pDisc = stack.portal(PositiveDisc);

    BOOST_CHECK_EQUAL(shell1.portal(NegativeDisc), nDisc);
    BOOST_CHECK_EQUAL(shell2.portal(NegativeDisc), nDisc);
    BOOST_CHECK_EQUAL(shell1.portal(PositiveDisc), pDisc);
    BOOST_CHECK_EQUAL(shell2.portal(PositiveDisc), pDisc);

    BOOST_CHECK_EQUAL(stack.portal(InnerCylinder),
                      shell1.portal(InnerCylinder));
    BOOST_CHECK_EQUAL(stack.portal(OuterCylinder),
                      shell2.portal(OuterCylinder));

    BOOST_CHECK_EQUAL(stack.portal(NegativePhiPlane), nullptr);
    BOOST_CHECK_EQUAL(stack.portalPtr(NegativePhiPlane), nullptr);

    BOOST_CHECK_EQUAL(stack.portal(PositivePhiPlane), nullptr);
    BOOST_CHECK_EQUAL(stack.portalPtr(PositivePhiPlane), nullptr);

    shell1 = SingleCylinderPortalShell{vol1};
    shell2 = SingleCylinderPortalShell{vol2};

    BOOST_CHECK_THROW(
        CylinderStackPortalShell(gctx, {&shell1, &shell2}, BinningValue::binZ),
        SurfaceMergingException);
  }

  BOOST_TEST_CONTEXT("rMin==0") {
    TrackingVolume vol1(
        Transform3::Identity(),
        std::make_shared<CylinderVolumeBounds>(0_mm, 100_mm, 100_mm));

    TrackingVolume vol2(
        Transform3::Identity(),
        std::make_shared<CylinderVolumeBounds>(100_mm, 150_mm, 100_mm));

    SingleCylinderPortalShell shell1{vol1};
    SingleCylinderPortalShell shell2{vol2};

    BOOST_CHECK_EQUAL(shell1.portal(InnerCylinder), nullptr);
    BOOST_CHECK_NE(shell1.portal(OuterCylinder), shell2.portal(InnerCylinder));

    CylinderStackPortalShell stack{
        gctx, {&shell1, &shell2}, BinningValue::binR};
    BOOST_CHECK_EQUAL(stack.size(), 4);

    // Internal cylinder portals have been fused
    BOOST_CHECK_EQUAL(shell1.portal(OuterCylinder),
                      shell2.portal(InnerCylinder));

    const auto* nDisc = stack.portal(NegativeDisc);
    const auto* pDisc = stack.portal(PositiveDisc);

    BOOST_CHECK_EQUAL(shell1.portal(NegativeDisc), nDisc);
    BOOST_CHECK_EQUAL(shell2.portal(NegativeDisc), nDisc);
    BOOST_CHECK_EQUAL(shell1.portal(PositiveDisc), pDisc);
    BOOST_CHECK_EQUAL(shell2.portal(PositiveDisc), pDisc);

    BOOST_CHECK_EQUAL(stack.portal(InnerCylinder), nullptr);
    BOOST_CHECK_EQUAL(stack.portal(OuterCylinder),
                      shell2.portal(OuterCylinder));

    BOOST_CHECK_EQUAL(stack.portal(NegativePhiPlane), nullptr);
    BOOST_CHECK_EQUAL(stack.portalPtr(NegativePhiPlane), nullptr);

    BOOST_CHECK_EQUAL(stack.portal(PositivePhiPlane), nullptr);
    BOOST_CHECK_EQUAL(stack.portalPtr(PositivePhiPlane), nullptr);

    shell1 = SingleCylinderPortalShell{vol1};
    shell2 = SingleCylinderPortalShell{vol2};

    BOOST_CHECK_THROW(
        CylinderStackPortalShell(gctx, {&shell1, &shell2}, BinningValue::binZ),
        std::invalid_argument);
  }
}

BOOST_AUTO_TEST_CASE(NestedStacks) {
  //   ^
  // r |    +---------------------------------+---------+
  //   |    |                                 |         |
  //   |    |                                 |         |
  //   |    |              vol2               |         |
  //   |    |                                 |         |
  //   |    |                                 |         |
  //   |    +---------------------------------+         |
  //   |    |                                 |         |
  //   |    |                                 |         |
  //   |    |               gap               |  vol3   |
  //   |    |                                 |         |
  //   |    |                                 |         |
  //   |    +---------------------------------+         |
  //   |    |                                 |         |
  //   |    |                                 |         |
  //   |    |              vol1               |         |
  //   |    |                                 |         |
  //   |    |                                 |         |
  //   |    +---------------------------------+---------+
  //   |
  //   +-------------------------------------------------->
  //                                                      z

  Transform3 base = Transform3::Identity();

  TrackingVolume vol1(
      base, std::make_shared<CylinderVolumeBounds>(23_mm, 48_mm, 200_mm),
      "PixelLayer0");

  TrackingVolume gap(
      base, std::make_shared<CylinderVolumeBounds>(48_mm, 250_mm, 200_mm),
      "Gap");

  TrackingVolume vol2(
      base, std::make_shared<CylinderVolumeBounds>(250_mm, 400_mm, 200_mm),
      "PixelLayer3");

  TrackingVolume vol3(
      base * Translation3{Vector3::UnitZ() * 300_mm},
      std::make_shared<CylinderVolumeBounds>(23_mm, 400_mm, 100_mm),
      "PixelEcPos");

  SingleCylinderPortalShell shell1{vol1};
  BOOST_CHECK(shell1.isValid());
  SingleCylinderPortalShell gapShell{gap};
  BOOST_CHECK(gapShell.isValid());
  SingleCylinderPortalShell shell2{vol2};
  BOOST_CHECK(shell2.isValid());

  CylinderStackPortalShell stack{
      gctx, {&shell1, &gapShell, &shell2}, BinningValue::binR};

  BOOST_CHECK(stack.isValid());

  SingleCylinderPortalShell shell3{vol3};
  BOOST_CHECK(shell3.isValid());

  CylinderStackPortalShell stack2{
      gctx, {&stack, &shell3}, BinningValue::binZ, *logger};
  BOOST_CHECK(stack2.isValid());

  using enum CylinderPortalShell::Face;

  auto lookup = [](auto& shell, CylinderPortalShell::Face face,
                   Vector3 position,
                   Vector3 direction) -> const TrackingVolume* {
    const auto* portal = shell.portal(face);
    BOOST_REQUIRE_NE(portal, nullptr);
    return portal->resolveVolume(gctx, position, direction).value();
  };

  // Interconnection in the r direction

  BOOST_CHECK_EQUAL(lookup(shell1, InnerCylinder, 23_mm * Vector3::UnitX(),
                           -Vector3::UnitX()),
                    nullptr);
  BOOST_CHECK_EQUAL(
      lookup(shell1, InnerCylinder, 23_mm * Vector3::UnitX(), Vector3::UnitX()),
      &vol1);

  BOOST_CHECK_EQUAL(
      lookup(shell1, OuterCylinder, 48_mm * Vector3::UnitX(), Vector3::UnitX()),
      &gap);

  BOOST_CHECK_EQUAL(lookup(gapShell, InnerCylinder, 48_mm * Vector3::UnitX(),
                           -Vector3::UnitX()),
                    &vol1);

  BOOST_CHECK_EQUAL(lookup(gapShell, OuterCylinder, 250_mm * Vector3::UnitX(),
                           Vector3::UnitX()),
                    &vol2);

  BOOST_CHECK_EQUAL(lookup(shell2, InnerCylinder, 250_mm * Vector3::UnitX(),
                           -Vector3::UnitX()),
                    &gap);

  BOOST_CHECK_EQUAL(lookup(shell2, OuterCylinder, 400_mm * Vector3::UnitX(),
                           Vector3::UnitX()),
                    nullptr);

  BOOST_CHECK_EQUAL(lookup(shell2, OuterCylinder, 400_mm * Vector3::UnitX(),
                           -Vector3::UnitX()),
                    &vol2);

  BOOST_CHECK_EQUAL(lookup(shell2, OuterCylinder, 400_mm * Vector3::UnitX(),
                           -Vector3::UnitX()),
                    &vol2);

  // Left connection

  BOOST_CHECK_EQUAL(lookup(shell1, NegativeDisc, Vector3(30_mm, 0, -200_mm),
                           -Vector3::UnitZ()),
                    nullptr);

  BOOST_CHECK_EQUAL(lookup(shell1, NegativeDisc, Vector3(30_mm, 0, -200_mm),
                           Vector3::UnitZ()),
                    &vol1);

  BOOST_CHECK_EQUAL(lookup(gapShell, NegativeDisc, Vector3(60_mm, 0, -200_mm),
                           -Vector3::UnitZ()),
                    nullptr);

  BOOST_CHECK_EQUAL(lookup(gapShell, NegativeDisc, Vector3(60_mm, 0, -200_mm),
                           Vector3::UnitZ()),
                    &gap);

  BOOST_CHECK_EQUAL(lookup(shell2, NegativeDisc, Vector3(300_mm, 0, -200_mm),
                           -Vector3::UnitZ()),
                    nullptr);

  BOOST_CHECK_EQUAL(lookup(shell2, NegativeDisc, Vector3(300_mm, 0, -200_mm),
                           Vector3::UnitZ()),
                    &vol2);

  // Right connection

  BOOST_CHECK_EQUAL(lookup(shell1, PositiveDisc, Vector3(30_mm, 0, 200_mm),
                           -Vector3::UnitZ()),
                    &vol1);

  BOOST_CHECK_EQUAL(
      lookup(shell1, PositiveDisc, Vector3(30_mm, 0, 200_mm), Vector3::UnitZ()),
      &vol3);

  BOOST_CHECK_EQUAL(lookup(gapShell, PositiveDisc, Vector3(60_mm, 0, 200_mm),
                           -Vector3::UnitZ()),
                    &gap);

  BOOST_CHECK_EQUAL(lookup(gapShell, PositiveDisc, Vector3(60_mm, 0, 200_mm),
                           Vector3::UnitZ()),
                    &vol3);

  BOOST_CHECK_EQUAL(lookup(shell2, PositiveDisc, Vector3(300_mm, 0, 200_mm),
                           -Vector3::UnitZ()),
                    &vol2);

  BOOST_CHECK_EQUAL(lookup(shell2, PositiveDisc, Vector3(300_mm, 0, 200_mm),
                           Vector3::UnitZ()),
                    &vol3);

  // Right outer connection

  BOOST_CHECK_EQUAL(lookup(shell3, PositiveDisc, Vector3(300_mm, 0, 400_mm),
                           -Vector3::UnitZ()),
                    &vol3);

  BOOST_CHECK_EQUAL(lookup(shell3, PositiveDisc, Vector3(300_mm, 0, 400_mm),
                           Vector3::UnitZ()),
                    nullptr);
}

BOOST_AUTO_TEST_CASE(ConnectOuter) {
  auto cyl1 = makeVolume(30_mm, 40_mm, 100_mm);
  auto cyl2 = makeVolume(0_mm, 50_mm, 110_mm);

  SingleCylinderPortalShell shell{cyl1};

  using enum CylinderPortalShell::Face;
  BOOST_CHECK_EQUAL(
      shell.portal(OuterCylinder)->getLink(Direction::AlongNormal), nullptr);
  BOOST_CHECK_EQUAL(
      shell.portal(InnerCylinder)->getLink(Direction::OppositeNormal), nullptr);
  BOOST_CHECK_EQUAL(shell.portal(PositiveDisc)->getLink(Direction::AlongNormal),
                    nullptr);
  BOOST_CHECK_EQUAL(
      shell.portal(NegativeDisc)->getLink(Direction::OppositeNormal), nullptr);

  shell.connectOuter(cyl2);

  BOOST_CHECK_NE(shell.portal(OuterCylinder)->getLink(Direction::AlongNormal),
                 nullptr);
  BOOST_CHECK_NE(
      shell.portal(InnerCylinder)->getLink(Direction::OppositeNormal), nullptr);
  BOOST_CHECK_NE(shell.portal(PositiveDisc)->getLink(Direction::AlongNormal),
                 nullptr);
  BOOST_CHECK_NE(shell.portal(NegativeDisc)->getLink(Direction::OppositeNormal),
                 nullptr);
}

BOOST_AUTO_TEST_CASE(RegisterInto) {
  using enum CylinderPortalShell::Face;
  TrackingVolume vol1(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(0_mm, 100_mm, 100_mm));

  SingleCylinderPortalShell shell{vol1};

  BOOST_CHECK_EQUAL(vol1.portals().size(), 0);

  shell.applyToVolume();
  BOOST_CHECK_EQUAL(vol1.portals().size(), 3);
}

BOOST_AUTO_TEST_SUITE_END()  // CylinderStack
BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
