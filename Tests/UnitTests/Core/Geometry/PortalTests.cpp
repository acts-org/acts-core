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
#include "Acts/Geometry/CompositePortalLink.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GridPortalLink.hpp"
#include "Acts/Geometry/Portal.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/TrivialPortalLink.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceMergingException.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

#include <stdexcept>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

auto logger = Acts::getDefaultLogger("UnitTests", Acts::Logging::VERBOSE);

struct Fixture {
  Logging::Level m_level;
  Fixture() {
    m_level = Acts::Logging::getFailureThreshold();
    Acts::Logging::setFailureThreshold(Acts::Logging::FATAL);
  }

  ~Fixture() { Acts::Logging::setFailureThreshold(m_level); }
};

std::shared_ptr<TrackingVolume> makeDummyVolume() {
  return std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));
}

GeometryContext gctx;

BOOST_FIXTURE_TEST_SUITE(Geometry, Fixture)

BOOST_AUTO_TEST_SUITE(Portals)
BOOST_AUTO_TEST_SUITE(Merging)

BOOST_AUTO_TEST_CASE(Cylinder) {
  auto vol1 = makeDummyVolume();
  vol1->setVolumeName("vol1");
  auto vol2 = makeDummyVolume();
  vol2->setVolumeName("vol2");

  auto cyl1 = Surface::makeShared<CylinderSurface>(
      Transform3{Translation3{Vector3::UnitZ() * -100_mm}}, 50_mm, 100_mm);

  auto cyl2 = Surface::makeShared<CylinderSurface>(
      Transform3{Translation3{Vector3::UnitZ() * 100_mm}}, 50_mm, 100_mm);

  Portal portal1{Direction::AlongNormal(),
                 std::make_unique<TrivialPortalLink>(cyl1, *vol1)};
  BOOST_CHECK(portal1.isValid());

  BOOST_CHECK_EQUAL(
      portal1
          .resolveVolume(gctx, Vector3{50_mm, 0_mm, -100_mm}, Vector3::UnitX())
          .value(),
      vol1.get());

  BOOST_CHECK_EQUAL(
      portal1
          .resolveVolume(gctx, Vector3{50_mm, 0_mm, -100_mm}, -Vector3::UnitX())
          .value(),
      nullptr);

  Portal portal2{Direction::AlongNormal(), cyl2, *vol2};
  BOOST_CHECK(portal2.isValid());

  BOOST_CHECK_EQUAL(
      portal2
          .resolveVolume(gctx, Vector3{50_mm, 0_mm, 100_mm}, -Vector3::UnitX())
          .value(),
      nullptr);

  BOOST_CHECK_EQUAL(
      portal2
          .resolveVolume(gctx, Vector3{50_mm, 0_mm, 100_mm}, Vector3::UnitX())
          .value(),
      vol2.get());

  Portal portal3{gctx, std::make_unique<TrivialPortalLink>(cyl2, *vol2),
                 nullptr};
  BOOST_CHECK(portal3.isValid());

  BOOST_CHECK_NE(portal3.getLink(Direction::AlongNormal()), nullptr);
  BOOST_CHECK_EQUAL(portal3.getLink(Direction::OppositeNormal()), nullptr);

  Portal portal4{gctx, nullptr,
                 std::make_unique<TrivialPortalLink>(cyl2, *vol2)};
  BOOST_CHECK(portal4.isValid());

  BOOST_CHECK_EQUAL(portal4.getLink(Direction::AlongNormal()), nullptr);
  BOOST_CHECK_NE(portal4.getLink(Direction::OppositeNormal()), nullptr);

  // Not mergeable because 1 has portal along but 4 has portal oppsite
  //         ^
  //         |
  //  portal1|              portal2
  // +-------+-------+  +  +---------------+
  // |               |     |               |
  // +---------------+     +-------+-------+
  //                               |
  //                               |
  //                               v
  BOOST_CHECK_THROW(
      Portal::merge(gctx, portal1, portal4, AxisDirection::AxisZ, *logger),
      PortalMergingException);

  // This call leaves both valid because the exception is thrown before the
  // pointers are moved
  BOOST_CHECK(portal1.isValid());
  BOOST_CHECK(portal2.isValid());

  BOOST_CHECK_EQUAL(
      portal2.resolveVolume(gctx, Vector3{50_mm, 0_mm, 50_mm}, Vector3::UnitX())
          .value(),
      vol2.get());

  BOOST_CHECK_EQUAL(
      portal2
          .resolveVolume(gctx, Vector3{50_mm, 0_mm, 50_mm}, -Vector3::UnitX())
          .value(),
      nullptr);

  // Cannot merge in AxisRPhi
  BOOST_CHECK_THROW(
      Portal::merge(gctx, portal1, portal2, AxisDirection::AxisRPhi, *logger),
      SurfaceMergingException);

  // The call above leaves both portals invalid because the exception is thrown
  // after the pointers are moved (during durface merging)
  BOOST_CHECK(!portal1.isValid());
  BOOST_CHECK(!portal2.isValid());

  //         ^                     ^
  //         |                     |
  //  portal1|              portal2|
  // +-------+-------+  +  +-------+-------+
  // |               |     |               |
  // +---------------+     +---------------+

  // Reset portals to valid to continue
  portal1 = Portal{gctx, {.alongNormal = {cyl1, *vol1}}};
  portal2 = Portal{gctx, {.alongNormal = {cyl2, *vol2}}};

  Portal merged12 =
      Portal::merge(gctx, portal1, portal2, AxisDirection::AxisZ, *logger);
  BOOST_CHECK_NE(merged12.getLink(Direction::AlongNormal()), nullptr);
  BOOST_CHECK_EQUAL(merged12.getLink(Direction::OppositeNormal()), nullptr);

  auto composite12 = dynamic_cast<const CompositePortalLink*>(
      merged12.getLink(Direction::AlongNormal()));
  BOOST_REQUIRE_NE(composite12, nullptr);

  BOOST_CHECK_EQUAL(
      merged12
          .resolveVolume(gctx, Vector3{50_mm, 0_mm, -50_mm}, Vector3::UnitX())
          .value(),
      vol1.get());

  BOOST_CHECK_EQUAL(
      merged12
          .resolveVolume(gctx, Vector3{50_mm, 0_mm, 50_mm}, Vector3::UnitX())
          .value(),
      vol2.get());

  BOOST_CHECK_EQUAL(
      merged12
          .resolveVolume(gctx, Vector3{50_mm, 0_mm, -50_mm}, -Vector3::UnitX())
          .value(),
      nullptr);

  BOOST_CHECK_EQUAL(
      merged12
          .resolveVolume(gctx, Vector3{50_mm, 0_mm, 50_mm}, -Vector3::UnitX())
          .value(),
      nullptr);

  portal1 = Portal{gctx, {.alongNormal = {cyl1, *vol1}}};

  // Can't merge with self
  BOOST_CHECK_THROW(
      Portal::merge(gctx, portal1, portal1, AxisDirection::AxisZ, *logger),
      PortalMergingException);

  // Can't merge because the surfaces are the same
  portal1 = Portal{gctx, {.alongNormal = {cyl1, *vol1}}};
  portal2 = Portal{gctx, {.alongNormal = {cyl1, *vol2}}};
  BOOST_CHECK_THROW(
      Portal::merge(gctx, portal1, portal2, AxisDirection::AxisZ, *logger),
      AssertionFailureException);

  // Can't merge because surface has material
  auto material = std::make_shared<HomogeneousSurfaceMaterial>(
      MaterialSlab::Nothing());  // vacuum
  cyl2->assignSurfaceMaterial(material);
  portal1 = Portal{gctx, {.alongNormal = {cyl1, *vol1}}};
  portal2 = Portal{gctx, {.alongNormal = {cyl2, *vol2}}};
  BOOST_CHECK_THROW(
      Portal::merge(gctx, portal1, portal2, AxisDirection::AxisZ, *logger),
      PortalMergingException);
}

BOOST_AUTO_TEST_CASE(Disc) {
  auto vol1 = makeDummyVolume();
  vol1->setVolumeName("vol1");
  auto vol2 = makeDummyVolume();
  vol2->setVolumeName("vol2");
  auto vol3 = makeDummyVolume();
  vol3->setVolumeName("vol3");
  auto vol4 = makeDummyVolume();
  vol4->setVolumeName("vol4");

  auto disc1 = Surface::makeShared<DiscSurface>(
      Transform3::Identity(), std::make_shared<RadialBounds>(50_mm, 100_mm));

  auto disc2 = Surface::makeShared<DiscSurface>(
      Transform3::Identity(), std::make_shared<RadialBounds>(100_mm, 150_mm));

  Portal portal1{
      gctx, {.alongNormal = {disc1, *vol1}, .oppositeNormal = {disc1, *vol2}}};

  Portal portal2{
      gctx, {.alongNormal = {disc2, *vol3}, .oppositeNormal = {disc2, *vol4}}};

  BOOST_CHECK(portal1.isValid());
  BOOST_CHECK(portal2.isValid());

  BOOST_CHECK_EQUAL(
      portal1.resolveVolume(gctx, Vector3{55_mm, 0_mm, 0_mm}, Vector3::UnitZ())
          .value(),
      vol1.get());
  BOOST_CHECK_EQUAL(
      portal1.resolveVolume(gctx, Vector3{55_mm, 0_mm, 0_mm}, -Vector3::UnitZ())
          .value(),
      vol2.get());

  BOOST_CHECK_EQUAL(
      portal2.resolveVolume(gctx, Vector3{105_mm, 0_mm, 0_mm}, Vector3::UnitZ())
          .value(),
      vol3.get());
  BOOST_CHECK_EQUAL(
      portal2
          .resolveVolume(gctx, Vector3{105_mm, 0_mm, 0_mm}, -Vector3::UnitZ())
          .value(),
      vol4.get());

  BOOST_CHECK_THROW(
      Portal::merge(gctx, portal1, portal2, AxisDirection::AxisZ, *logger),
      AssertionFailureException);

  BOOST_CHECK(portal1.isValid());
  BOOST_CHECK(portal2.isValid());

  BOOST_CHECK_THROW(
      Portal::merge(gctx, portal1, portal2, AxisDirection::AxisPhi, *logger),
      SurfaceMergingException);

  // Portals not valid anymore because they were moved before the exception was
  // thrown
  BOOST_CHECK(!portal1.isValid());
  BOOST_CHECK(!portal2.isValid());

  // recreate them
  portal1 = Portal{
      gctx, {.alongNormal = {disc1, *vol1}, .oppositeNormal = {disc1, *vol2}}};

  portal2 = Portal{
      gctx, {.alongNormal = {disc2, *vol3}, .oppositeNormal = {disc2, *vol4}}};

  //         ^                     ^
  //         |                     |
  //  portal1|              portal2|
  // +-------+-------+     +-------+-------+
  // |               |  +  |               |
  // +-------+-------+     +-------+-------+
  //         |                     |
  //         |                     |
  //         v                     v
  Portal merged12 =
      Portal::merge(gctx, portal1, portal2, AxisDirection::AxisR, *logger);

  BOOST_CHECK_EQUAL(
      merged12.resolveVolume(gctx, Vector3{55_mm, 0_mm, 0_mm}, Vector3::UnitZ())
          .value(),
      vol1.get());
  BOOST_CHECK_EQUAL(
      merged12
          .resolveVolume(gctx, Vector3{55_mm, 0_mm, 0_mm}, -Vector3::UnitZ())
          .value(),
      vol2.get());

  BOOST_CHECK_EQUAL(
      merged12
          .resolveVolume(gctx, Vector3{105_mm, 0_mm, 0_mm}, Vector3::UnitZ())
          .value(),
      vol3.get());
  BOOST_CHECK_EQUAL(
      merged12
          .resolveVolume(gctx, Vector3{105_mm, 0_mm, 0_mm}, -Vector3::UnitZ())
          .value(),
      vol4.get());

  // Can't merge because surface has material
  auto material = std::make_shared<HomogeneousSurfaceMaterial>(
      MaterialSlab::Nothing());  // vacuum
  disc2->assignSurfaceMaterial(material);
  portal1 = Portal{
      gctx, {.alongNormal = {disc1, *vol1}, .oppositeNormal = {disc1, *vol2}}};
  portal2 = Portal{
      gctx, {.alongNormal = {disc2, *vol3}, .oppositeNormal = {disc2, *vol4}}};
  BOOST_CHECK_THROW(
      Portal::merge(gctx, portal1, portal2, AxisDirection::AxisR, *logger),
      PortalMergingException);
}

BOOST_AUTO_TEST_SUITE_END()  // Merging

BOOST_AUTO_TEST_SUITE(Fusing)

BOOST_AUTO_TEST_CASE(Separated) {
  auto vol1 = makeDummyVolume();
  vol1->setVolumeName("vol1");
  auto vol2 = makeDummyVolume();
  vol2->setVolumeName("vol2");

  auto cyl1 = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                   50_mm, 100_mm);

  auto cyl2 = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                   60_mm, 100_mm);

  Portal portal1{gctx, {.oppositeNormal = {cyl1, *vol1}}};

  Portal portal2{gctx, {.alongNormal = {cyl2, *vol2}}};

  BOOST_CHECK(portal1.isValid());
  BOOST_CHECK(portal2.isValid());

  // Surfaces have a 10mm gap in r
  BOOST_CHECK_THROW(Portal::fuse(gctx, portal1, portal2, *logger),
                    PortalFusingException);

  BOOST_CHECK(portal1.isValid());
  BOOST_CHECK(portal2.isValid());

  // Same way can't set cyl2 as other link
  BOOST_CHECK_THROW(
      portal1.setLink(gctx, Direction::AlongNormal(), cyl2, *vol2),
      PortalFusingException);
  BOOST_CHECK_EQUAL(portal1.getLink(Direction::AlongNormal()), nullptr);

  Portal portal1b{gctx, {.oppositeNormal = {cyl1, *vol1}}};
  BOOST_CHECK(portal1b.isValid());

  //    portal1     portal1b
  //      +---+        +---+
  //      |   |        |   |
  //      |   |        |   |
  // <----+   | + <----+   |
  //      |   |        |   |
  //      |   |        |   |
  //      +---+        +---+
  BOOST_CHECK_THROW(Portal::fuse(gctx, portal1, portal1b, *logger),
                    PortalFusingException);
  BOOST_CHECK(portal1.isValid());
  BOOST_CHECK(portal1b.isValid());

  auto disc1 = Surface::makeShared<DiscSurface>(
      Transform3::Identity(), std::make_shared<RadialBounds>(50_mm, 100_mm));

  auto disc2 = Surface::makeShared<DiscSurface>(
      Transform3{Translation3{Vector3{0, 0, 5_mm}}},
      std::make_shared<RadialBounds>(50_mm, 100_mm));

  // portal2       portal2b
  //   +---+          +---+
  //   |   |          |   |
  //   |   |          |   |
  //   |   +---->  +  |   +---->
  //   |   |          |   |
  //   |   |          |   |
  //   +---+          +---+
  Portal portal2b{gctx, {.alongNormal = {disc2, *vol2}}};

  BOOST_CHECK_THROW(Portal::fuse(gctx, portal2, portal2b, *logger),
                    PortalFusingException);
  BOOST_CHECK(portal1.isValid());
  BOOST_CHECK(portal2.isValid());

  //    portal2     portal2c
  //      +---+        +---+
  //      |   |        |   |
  //      |   |        |   |
  // <----+   | + <----+   +---->
  //      |   |        |   |
  //      |   |        |   |
  //      +---+        +---+
  Portal portal2c{
      gctx, {.alongNormal = {disc2, *vol1}, .oppositeNormal = {disc2, *vol2}}};
  BOOST_CHECK(portal2c.isValid());

  BOOST_CHECK_THROW(Portal::fuse(gctx, portal2, portal2c, *logger),
                    PortalFusingException);
  BOOST_CHECK(portal2.isValid());
  BOOST_CHECK(portal2c.isValid());
}

BOOST_AUTO_TEST_CASE(Success) {
  auto vol1 = makeDummyVolume();
  vol1->setVolumeName("vol1");
  auto vol2 = makeDummyVolume();
  vol2->setVolumeName("vol2");

  auto cyl1 = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                   50_mm, 100_mm);

  auto cyl2 = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                   50_mm, 100_mm);

  BOOST_CHECK(*cyl1 == *cyl2);

  //    portal1   portal2
  //      +---+   +---+
  //      |   |   |   |
  //      |   |   |   |
  // <----+   | + |   +---->
  //      |   |   |   |
  //      |   |   |   |
  //      +---+   +---+
  Portal portal1{gctx, {.oppositeNormal = {cyl1, *vol1}}};
  BOOST_CHECK_EQUAL(&portal1.getLink(Direction::OppositeNormal())->surface(),
                    cyl1.get());

  Portal portal2{gctx, {.alongNormal = {cyl2, *vol2}}};

  BOOST_CHECK(portal1.isValid());
  BOOST_CHECK(portal2.isValid());

  Portal portal3 = Portal::fuse(gctx, portal1, portal2, *logger);
  // Input portals get invalidated by the fuse
  BOOST_CHECK(!portal1.isValid());
  BOOST_CHECK(!portal2.isValid());
  BOOST_CHECK(portal3.isValid());

  BOOST_CHECK_EQUAL(portal3.surface().surfaceMaterial(), nullptr);

  // Portal surface is set to the one from "along", because it gets set first
  BOOST_CHECK_EQUAL(&portal3.surface(), cyl2.get());
  // "Opposite" gets the already-set surface set as well
  BOOST_CHECK_EQUAL(&portal3.getLink(Direction::OppositeNormal())->surface(),
                    cyl2.get());
}

BOOST_AUTO_TEST_CASE(Material) {
  auto vol1 = makeDummyVolume();
  auto vol2 = makeDummyVolume();

  auto cyl1 = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                   50_mm, 100_mm);

  auto cyl2 = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                   50_mm, 100_mm);

  //    portal1   portal2
  //      +---+   +---+
  //      |   |   |   |
  //      |   |   |   |
  // <----+   | + |   +---->
  //      |   |   |   |
  //      |   |   |   |
  //      +---+   +---+
  Portal portal1{gctx, {.oppositeNormal = {cyl1, *vol1}}};
  Portal portal2{gctx, {.alongNormal = {cyl2, *vol2}}};

  auto material = std::make_shared<HomogeneousSurfaceMaterial>(
      MaterialSlab::Nothing());  // vacuum

  cyl1->assignSurfaceMaterial(material);

  Portal portal12 = Portal::fuse(gctx, portal1, portal2, *logger);

  // cyl1 had material, so this surface needs to be retained
  BOOST_CHECK_EQUAL(&portal12.surface(), cyl1.get());
  BOOST_CHECK_EQUAL(portal12.surface().surfaceMaterial(), material.get());

  // Reset portals
  portal1 = Portal{gctx, {.oppositeNormal = {cyl1, *vol1}}};
  portal2 = Portal{gctx, {.alongNormal = {cyl2, *vol2}}};
  cyl2->assignSurfaceMaterial(material);

  // Both have material, this should fail
  BOOST_CHECK_THROW(Portal::fuse(gctx, portal1, portal2, *logger),
                    PortalFusingException);
  // Portals should stay valid
  BOOST_CHECK(portal1.isValid());
  BOOST_CHECK(portal2.isValid());

  cyl1->assignSurfaceMaterial(nullptr);

  portal12 = Portal::fuse(gctx, portal1, portal2, *logger);

  // cyl2 had material, so this surface needs to be retained
  BOOST_CHECK_EQUAL(&portal12.surface(), cyl2.get());
  BOOST_CHECK_EQUAL(portal12.surface().surfaceMaterial(), material.get());
}

BOOST_AUTO_TEST_CASE(GridCreationOnFuse) {
  Transform3 base = Transform3::Identity();

  auto vol1 = std::make_shared<TrackingVolume>(
      base, std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto vol2 = std::make_shared<TrackingVolume>(
      base, std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto vol3 = std::make_shared<TrackingVolume>(
      base, std::make_shared<CylinderVolumeBounds>(40_mm, 50_mm, 100_mm));

  auto vol4 = std::make_shared<TrackingVolume>(
      base, std::make_shared<CylinderVolumeBounds>(40_mm, 50_mm, 100_mm));

  auto disc1 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm, 60_mm);

  auto disc2 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 60_mm, 90_mm);

  auto disc3 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 90_mm, 120_mm);

  auto trivial1 = std::make_unique<TrivialPortalLink>(disc1, *vol1);
  BOOST_REQUIRE(trivial1);
  auto trivial2 = std::make_unique<TrivialPortalLink>(disc2, *vol2);
  BOOST_REQUIRE(trivial2);
  auto trivial3 = std::make_unique<TrivialPortalLink>(disc3, *vol3);
  BOOST_REQUIRE(trivial3);

  std::vector<std::unique_ptr<PortalLinkBase>> links;
  links.push_back(std::move(trivial1));
  links.push_back(std::move(trivial2));
  links.push_back(std::move(trivial3));

  auto composite = std::make_unique<CompositePortalLink>(std::move(links),
                                                         AxisDirection::AxisR);

  auto discOpposite =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm, 120_mm);

  auto trivialOpposite =
      std::make_unique<TrivialPortalLink>(discOpposite, *vol4);

  Portal aPortal{gctx, std::move(composite), nullptr};
  Portal bPortal{gctx, nullptr, std::move(trivialOpposite)};

  Portal fused = Portal::fuse(gctx, aPortal, bPortal, *logger);

  BOOST_CHECK_NE(dynamic_cast<const TrivialPortalLink*>(
                     fused.getLink(Direction::OppositeNormal())),
                 nullptr);

  const auto* grid = dynamic_cast<const GridPortalLink*>(
      fused.getLink(Direction::AlongNormal()));
  BOOST_REQUIRE_NE(grid, nullptr);

  BOOST_CHECK_EQUAL(grid->grid().axes().front()->getNBins(), 3);
}

BOOST_AUTO_TEST_SUITE_END()  // Fusing

BOOST_AUTO_TEST_CASE(Construction) {
  auto vol1 = makeDummyVolume();

  // Displaced surfaces fail
  auto disc1 = Surface::makeShared<DiscSurface>(
      Transform3::Identity(), std::make_shared<RadialBounds>(50_mm, 100_mm));

  auto disc2 = Surface::makeShared<DiscSurface>(
      Transform3{Translation3{Vector3{0, 0, 5_mm}}},
      std::make_shared<RadialBounds>(50_mm, 100_mm));

  BOOST_CHECK_THROW(std::make_unique<Portal>(
                        gctx, std::make_unique<TrivialPortalLink>(disc1, *vol1),
                        std::make_unique<TrivialPortalLink>(disc2, *vol1)),
                    PortalFusingException);

  BOOST_CHECK_THROW((Portal{gctx, nullptr, nullptr}), std::invalid_argument);
  BOOST_CHECK_THROW(Portal(gctx, {}), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(InvalidConstruction) {
  BOOST_CHECK_THROW(Portal(Direction::AlongNormal(), nullptr),
                    std::invalid_argument);

  auto vol1 = makeDummyVolume();

  BOOST_CHECK_THROW(Portal(Direction::AlongNormal(), nullptr, *vol1),
                    std::invalid_argument);

  auto disc1 = Surface::makeShared<DiscSurface>(
      Transform3::Identity(), std::make_shared<RadialBounds>(50_mm, 100_mm));
  Portal portal(Direction::AlongNormal(), disc1, *vol1);

  BOOST_CHECK_THROW(portal.setLink(gctx, Direction::AlongNormal(), nullptr),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(PortalFill) {
  auto vol1 = makeDummyVolume();
  auto vol2 = makeDummyVolume();

  auto cyl1 = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                   50_mm, 100_mm);

  Portal portal1{gctx, {.oppositeNormal = {cyl1, *vol1}}};
  Portal portal2{gctx, {.alongNormal = {cyl1, *vol2}}};

  // Fuse these to make portal 1 and 2 empty
  Portal::fuse(gctx, portal1, portal2, *logger);

  BOOST_CHECK_THROW(portal1.fill(*vol2), std::logic_error);

  portal1 = Portal{gctx, {.oppositeNormal = {cyl1, *vol1}}};
  portal2 = Portal{gctx, {.alongNormal = {cyl1, *vol2}}};

  BOOST_CHECK_EQUAL(portal1.getLink(Direction::AlongNormal()), nullptr);
  BOOST_CHECK_NE(portal1.getLink(Direction::OppositeNormal()), nullptr);

  portal1.fill(*vol2);
  BOOST_CHECK_NE(portal1.getLink(Direction::AlongNormal()), nullptr);
  BOOST_CHECK_NE(portal1.getLink(Direction::OppositeNormal()), nullptr);

  BOOST_CHECK_THROW(portal1.fill(*vol2), std::logic_error);
}

BOOST_AUTO_TEST_SUITE_END()  // Portals

BOOST_AUTO_TEST_SUITE_END()  // Geometry

}  // namespace Acts::Test
