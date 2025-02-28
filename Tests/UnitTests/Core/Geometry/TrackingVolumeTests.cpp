// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/Portal.hpp"
#include "Acts/Geometry/PortalLinkBase.hpp"
#include "Acts/Geometry/TrackingGeometryVisitor.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include "LayerStub.hpp"

using namespace Acts::UnitLiterals;

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Geometry)
BOOST_AUTO_TEST_SUITE(TrackingVolumeTests)

std::size_t countVolumes(const TrackingVolume& tv) {
  std::size_t count = 0;
  tv.visitVolumes([&count](const auto&) { ++count; });
  return count;
}

BOOST_AUTO_TEST_CASE(TrackigVolumeChildren) {
  auto cylBounds =
      std::make_shared<Acts::CylinderVolumeBounds>(10_mm, 20_mm, 100_mm);

  TrackingVolume tv{Transform3::Identity(), cylBounds};

  BOOST_CHECK(tv.volumes().empty());
  BOOST_CHECK_EQUAL(countVolumes(tv), 1);

  auto& child1 = tv.addVolume(
      std::make_unique<TrackingVolume>(Transform3::Identity(), cylBounds));

  BOOST_CHECK_EQUAL(tv.volumes().size(), 1);

  auto it = tv.volumes().begin();
  static_assert(std::is_same_v<decltype(*it), TrackingVolume&>);

  const auto& tvConst = tv;
  auto cit = tvConst.volumes().begin();
  static_assert(std::is_same_v<decltype(*cit), const TrackingVolume&>);

  BOOST_CHECK_EQUAL(&*it, &child1);

  BOOST_CHECK_EQUAL(countVolumes(tv), 2);

  tv.addVolume(
      std::make_unique<TrackingVolume>(Transform3::Identity(), cylBounds));

  BOOST_CHECK_EQUAL(countVolumes(tv), 3);
}

namespace {
void testVisitor(auto visitor) {
  BOOST_CHECK(!visitor.m_surfaceCalled);
  BOOST_CHECK(!visitor.m_volumeCalled);
  BOOST_CHECK(!visitor.m_boundaryCalled);
  BOOST_CHECK(!visitor.m_layerCalled);
  BOOST_CHECK(!visitor.m_portalCalled);

  auto surface = Surface::makeShared<PlaneSurface>(
      Transform3::Identity(), std::make_shared<RectangleBounds>(10_mm, 10_mm));

  visitor.visitSurface(*surface);
  BOOST_CHECK(visitor.m_surfaceCalled);

  auto cylBounds =
      std::make_shared<Acts::CylinderVolumeBounds>(10_mm, 20_mm, 100_mm);

  TrackingVolume tv{Transform3::Identity(), cylBounds};
  visitor.visitVolume(tv);

  BOOST_CHECK(visitor.m_volumeCalled);

  BoundarySurfaceT<TrackingVolume> boundary{surface, &tv, nullptr};
  visitor.visitBoundarySurface(boundary);
  BOOST_CHECK(visitor.m_boundaryCalled);

  LayerStub layer{nullptr};
  visitor.visitLayer(layer);
  BOOST_CHECK(visitor.m_layerCalled);

  Portal portal{Direction::AlongNormal(), surface, tv};
  visitor.visitPortal(portal);
  BOOST_CHECK(visitor.m_portalCalled);

  visitor.m_volumeCalled = false;
  tv.apply(visitor);
  BOOST_CHECK(visitor.m_volumeCalled);
}
}  // namespace

BOOST_AUTO_TEST_CASE(Visit) {
  struct Visitor : public TrackingGeometryVisitor {
    void visitSurface(const Surface& /*surface*/) override {
      m_surfaceCalled = true;
    }

    void visitVolume(const TrackingVolume& /*volume*/) override {
      m_volumeCalled = true;
    }

    void visitBoundarySurface(
        const BoundarySurfaceT<TrackingVolume>& /*boundary*/) override {
      m_boundaryCalled = true;
    }

    void visitLayer(const Layer& /*layer*/) override { m_layerCalled = true; }

    void visitPortal(const Portal& /*portal*/) override {
      m_portalCalled = true;
    }

    bool m_surfaceCalled = false;
    bool m_volumeCalled = false;
    bool m_boundaryCalled = false;
    bool m_layerCalled = false;
    bool m_portalCalled = false;
  };

  testVisitor(Visitor{});
}

BOOST_AUTO_TEST_CASE(VisitMutable) {
  struct MutableVisitor : public TrackingGeometryMutableVisitor {
    void visitSurface(Surface& /*surface*/) override { m_surfaceCalled = true; }

    void visitVolume(TrackingVolume& /*volume*/) override {
      m_volumeCalled = true;
    }

    void visitBoundarySurface(
        BoundarySurfaceT<TrackingVolume>& /*boundary*/) override {
      m_boundaryCalled = true;
    }

    void visitLayer(Layer& /*layer*/) override { m_layerCalled = true; }

    void visitPortal(Portal& /*portal*/) override { m_portalCalled = true; }

    bool m_surfaceCalled = false;
    bool m_volumeCalled = false;
    bool m_boundaryCalled = false;
    bool m_layerCalled = false;
    bool m_portalCalled = false;
  };

  testVisitor(MutableVisitor{});
}

BOOST_AUTO_TEST_CASE(VisitLambda) {
  bool surfaceCalled = false;
  bool volumeCalled = false;
  bool boundaryCalled = false;
  bool layerCalled = false;
  bool portalCalled = false;

  auto visitor = detail::TrackingGeometryLambdaVisitor{overloaded{
      [&](const Surface& /*surface*/) { surfaceCalled = true; },
      [&](const TrackingVolume& /*volume*/) { volumeCalled = true; },
      [&](const BoundarySurfaceT<TrackingVolume>& /*boundary*/) {
        boundaryCalled = true;
      },
      [&](const Layer& /*layer*/) { layerCalled = true; },
      [&](const Portal& /*portal*/) { portalCalled = true; },
  }};

  auto surface = Surface::makeShared<PlaneSurface>(
      Transform3::Identity(), std::make_shared<RectangleBounds>(10_mm, 10_mm));

  visitor.visitSurface(*surface);
  BOOST_CHECK(surfaceCalled);

  auto cylBounds =
      std::make_shared<Acts::CylinderVolumeBounds>(10_mm, 20_mm, 100_mm);

  TrackingVolume tv{Transform3::Identity(), cylBounds};
  visitor.visitVolume(tv);

  BOOST_CHECK(volumeCalled);

  BoundarySurfaceT<TrackingVolume> boundary{surface, &tv, nullptr};
  visitor.visitBoundarySurface(boundary);
  BOOST_CHECK(boundaryCalled);

  LayerStub layer{nullptr};
  visitor.visitLayer(layer);
  BOOST_CHECK(layerCalled);

  Portal portal{Direction::AlongNormal(), surface, tv};
  visitor.visitPortal(portal);
  BOOST_CHECK(portalCalled);

  volumeCalled = false;
  tv.apply(visitor);
  BOOST_CHECK(volumeCalled);
}

BOOST_AUTO_TEST_CASE(VisitLambdaMutable) {
  bool surfaceCalled = false;
  bool volumeCalled = false;
  bool boundaryCalled = false;
  bool layerCalled = false;
  bool portalCalled = false;

  auto overload = overloaded{
      [&](Surface& /*surface*/) { surfaceCalled = true; },
      [&](TrackingVolume& /*volume*/) { volumeCalled = true; },
      [&](BoundarySurfaceT<TrackingVolume>& /*boundary*/) {
        boundaryCalled = true;
      },
      [&](Layer& /*layer*/) { layerCalled = true; },
      [&](Portal& /*portal*/) { portalCalled = true; },
  };

  auto visitor = detail::TrackingGeometryLambdaMutableVisitor{overload};

  auto surface = Surface::makeShared<PlaneSurface>(
      Transform3::Identity(), std::make_shared<RectangleBounds>(10_mm, 10_mm));

  visitor.visitSurface(*surface);
  BOOST_CHECK(surfaceCalled);

  auto cylBounds =
      std::make_shared<Acts::CylinderVolumeBounds>(10_mm, 20_mm, 100_mm);

  TrackingVolume tv{Transform3::Identity(), cylBounds};
  visitor.visitVolume(tv);

  BOOST_CHECK(volumeCalled);

  BoundarySurfaceT<TrackingVolume> boundary{surface, &tv, nullptr};
  visitor.visitBoundarySurface(boundary);
  BOOST_CHECK(boundaryCalled);

  LayerStub layer{nullptr};
  visitor.visitLayer(layer);
  BOOST_CHECK(layerCalled);

  Portal portal{Direction::AlongNormal(), surface, tv};
  visitor.visitPortal(portal);
  BOOST_CHECK(portalCalled);

  volumeCalled = false;
  tv.apply(overload);
  BOOST_CHECK(volumeCalled);

  volumeCalled = false;
  tv.apply([&](Volume& /*volume*/) { volumeCalled = true; });
  BOOST_CHECK(volumeCalled);
}

BOOST_AUTO_TEST_CASE(CallableWithAny) {
  // Test callableWithAny
  static_assert(
      detail::callableWithAny<decltype([](Surface& /*surface*/) {})>());
  static_assert(
      detail::callableWithAny<decltype([](const Surface& /*surface*/) {})>());
  static_assert(detail::callableWithAny<decltype([](Portal& /*portal*/) {})>());
  static_assert(
      detail::callableWithAny<decltype([](const Portal& /*portal*/) {})>());
  static_assert(
      detail::callableWithAny<decltype([](TrackingVolume& /*volume*/) {})>());
  static_assert(!detail::callableWithAny<decltype([](int /*x*/) {})>());
  static_assert(!detail::callableWithAny<decltype([](void* /*ptr*/) {})>());

  // Test callableWithAnyConst specifically
  static_assert(detail::callableWithAnyConst<
                decltype([](const Surface& /*surface*/) {})>());
  static_assert(detail::callableWithAnyConst<
                decltype([](const TrackingVolume& /*volume*/) {})>());
  static_assert(
      detail::callableWithAnyConst<decltype([](const Layer& /*layer*/) {})>());
  static_assert(!detail::callableWithAnyConst<decltype([](int /*x*/) {})>());
  static_assert(
      !detail::callableWithAnyConst<decltype([](void* /*ptr*/) {})>());

  // Test callableWithAnyMutable specifically
  static_assert(
      detail::callableWithAnyMutable<decltype([](Surface& /*surface*/) {})>());
  static_assert(detail::callableWithAnyMutable<
                decltype([](TrackingVolume& /*volume*/) {})>());
  static_assert(
      detail::callableWithAnyMutable<decltype([](Layer& /*layer*/) {})>());
  static_assert(!detail::callableWithAnyMutable<decltype([](int /*x*/) {})>());
  static_assert(
      !detail::callableWithAnyMutable<decltype([](void* /*ptr*/) {})>());

  // Test mixed const/non-const overloads
  static_assert(detail::callableWithAny<decltype(overloaded{
                    [](Surface& /*surface*/) {},
                    [](const TrackingVolume& /*volume*/) {},
                })>());

  // Test with unrelated overloads
  static_assert(!detail::callableWithAny<decltype(overloaded{
                    [](int /*x*/) {},
                    [](double /*y*/) {},
                    [](const std::string& /*s*/) {},
                })>());

  // Test with mix of geometry and unrelated overloads
  static_assert(detail::callableWithAny<decltype(overloaded{
                    [](int /*x*/) {},
                    [](Surface& /*surface*/) {},
                    [](const std::string& /*s*/) {},
                })>());

  static_assert(detail::callableWithAny<decltype(overloaded{
                    [](void* /*ptr*/) {},
                    [](const Portal& /*portal*/) {},
                    [](double /*d*/) {},
                })>());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
