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

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CompositePortalLink.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GridPortalLink.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/TrivialPortalLink.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/SurfaceMergingException.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

#include <cstdio>
#include <iostream>
#include <memory>
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

template <typename T>
std::unique_ptr<T> copy(const std::unique_ptr<T>& p) {
  return std::make_unique<T>(*p);
}

template <typename link_t>
void visitBins(const link_t& link,
               const std::function<void(const TrackingVolume*)>& func) {
  auto& grid = link.grid();
  auto loc = grid.numLocalBins();
  if constexpr (std::decay_t<decltype(grid)>::DIM == 1) {
    for (std::size_t i = 1; i <= loc[0]; i++) {
      func(grid.atLocalBins({i}));
    }
  } else {
    for (std::size_t i = 1; i <= loc[0]; i++) {
      for (std::size_t j = 1; j <= loc[1]; j++) {
        func(grid.atLocalBins({i, j}));
      }
    }
  }
}

BOOST_FIXTURE_TEST_SUITE(Geometry, Fixture)

BOOST_AUTO_TEST_SUITE(GridConstruction)

BOOST_AUTO_TEST_CASE(Cylinder) {
  BOOST_TEST_CONTEXT("1D") {
    auto cyl = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                    30_mm, 100_mm);

    // Volume for bin testing
    auto vol = std::make_shared<TrackingVolume>(
        Transform3::Identity(),
        std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

    // Incompatible binning
    BOOST_CHECK_THROW(
        GridPortalLink::make(cyl, BinningValue::binZ, Axis{AxisBound, 0, 5, 5}),
        std::invalid_argument);

    auto grid1dCyl = GridPortalLink::make(cyl, BinningValue::binZ,
                                          Axis{AxisBound, -100_mm, 100_mm, 10});
    BOOST_REQUIRE(grid1dCyl);
    grid1dCyl->setVolume(vol.get());

    // Throws because non-closed axis
    BOOST_CHECK_THROW(GridPortalLink::make(cyl, BinningValue::binRPhi,
                                           Axis{AxisBound, -180_degree * 30_mm,
                                                180_degree * 30_mm, 10}),
                      std::invalid_argument);

    auto grid1dCylRPhi = GridPortalLink::make(
        cyl, BinningValue::binRPhi,
        Axis{AxisClosed, -180_degree * 30_mm, 180_degree * 30_mm, 10});
    BOOST_REQUIRE_NE(grid1dCylRPhi, nullptr);
    grid1dCylRPhi->setVolume(vol.get());

    Axis axisExpected{AxisClosed, -180_degree * 30_mm, 180_degree * 30_mm, 10};
    BOOST_CHECK_EQUAL(grid1dCylRPhi->grid().axes().size(), 1);
    const auto& axis = *grid1dCylRPhi->grid().axes().front();
    BOOST_CHECK_EQUAL(axis, axisExpected);

    // Another cylinder, shifted in z
    auto cyl2 = Surface::makeShared<CylinderSurface>(
        Transform3{Translation3{Vector3::UnitZ() * 150_mm}}, 30_mm, 50_mm);

    auto grid1dCyl2 = GridPortalLink::make(cyl2, BinningValue::binZ,
                                           Axis{AxisBound, -50_mm, 50_mm, 5});

    // Test exception on cylinder with non-zero average phi
    auto cylNonZeroAverage = Surface::makeShared<CylinderSurface>(
        Transform3::Identity(), 30_mm, 100_mm, 20_degree, 45_degree);
    BOOST_CHECK_THROW(
        GridPortalLink::make(cylNonZeroAverage, BinningValue::binZ,
                             Axis{AxisBound, -100_mm, 100_mm, 10}),
        std::invalid_argument);

    auto checkAllBins = [&](const auto& link) {
      visitBins(link, [&](const TrackingVolume* content) {
        BOOST_CHECK_EQUAL(content, vol.get());
      });
    };

    checkAllBins(*grid1dCyl);
    checkAllBins(*grid1dCylRPhi);

    // Extend to a 2D grid with auto phi binning

    auto grid2dCyl1 = grid1dCyl->extendTo2d(nullptr);
    BOOST_REQUIRE(grid2dCyl1);
    BOOST_CHECK_EQUAL(grid2dCyl1->grid().axes().size(), 2);
    BOOST_CHECK_EQUAL(grid2dCyl1->surface().bounds(), cyl->bounds());
    const auto* axis1 = grid2dCyl1->grid().axes().front();
    const auto* axis2 = grid2dCyl1->grid().axes().back();

    Axis axis1Expected{AxisClosed, -M_PI * 30_mm, M_PI * 30_mm, 1};
    BOOST_CHECK_EQUAL(*axis1, axis1Expected);
    Axis axis2Expected{AxisBound, -100_mm, 100_mm, 10};
    BOOST_CHECK_EQUAL(*axis2, axis2Expected);

    auto& concrete = dynamic_cast<
        GridPortalLinkT<decltype(axis1Expected), decltype(axis2Expected)>&>(
        *grid2dCyl1);

    checkAllBins(concrete);

    Axis axis1Explicit{AxisClosed, -M_PI * 30_mm, M_PI * 30_mm, 13};
    auto grid2dCyl1Explicit = grid1dCyl->extendTo2d(&axis1Explicit);
    BOOST_REQUIRE(grid2dCyl1Explicit);
    BOOST_CHECK_EQUAL(grid2dCyl1Explicit->grid().axes().size(), 2);
    axis1 = grid2dCyl1Explicit->grid().axes().front();
    axis2 = grid2dCyl1Explicit->grid().axes().back();

    BOOST_CHECK_EQUAL(*axis1, axis1Explicit);
    BOOST_CHECK_EQUAL(*axis2, axis2Expected);

    auto& concrete2 = dynamic_cast<
        GridPortalLinkT<decltype(axis1Explicit), decltype(axis2Expected)>&>(
        *grid2dCyl1Explicit);

    checkAllBins(concrete2);

    auto cylPhi = Surface::makeShared<CylinderSurface>(
        Transform3::Identity(), 30_mm, 100_mm, 45_degree);
    std::unique_ptr<GridPortalLink> grid1dCylPhi = GridPortalLink::make(
        cylPhi, BinningValue::binZ, Axis{AxisBound, -100_mm, 100_mm, 10});

    grid1dCylPhi->setVolume(vol.get());

    // Check that phi sector portal does not accept closed axis
    BOOST_CHECK_THROW(GridPortalLink::make(cylPhi, BinningValue::binRPhi,
                                           Axis{AxisClosed, -45_degree * 30_mm,
                                                45_degree * 30_mm, 10}),
                      std::invalid_argument);

    auto grid2dCylPhi = grid1dCylPhi->extendTo2d(nullptr);
    BOOST_CHECK_EQUAL(grid2dCylPhi->grid().axes().size(), 2);
    BOOST_CHECK_EQUAL(grid2dCylPhi->surface().bounds(), cylPhi->bounds());
    const auto* axis1Phi = grid2dCylPhi->grid().axes().front();
    const auto* axis2Phi = grid2dCylPhi->grid().axes().back();

    Axis axis1PhiExpected{AxisBound, -45_degree * 30_mm, 45_degree * 30_mm, 1};
    BOOST_CHECK_EQUAL(*axis1Phi, axis1PhiExpected);
    Axis axis2PhiExpected{AxisBound, -100_mm, 100_mm, 10};
    BOOST_CHECK_EQUAL(*axis2Phi, axis2PhiExpected);

    auto& concrete3 =
        dynamic_cast<GridPortalLinkT<decltype(axis1PhiExpected),
                                     decltype(axis2PhiExpected)>&>(
            *grid2dCylPhi);

    checkAllBins(concrete3);

    Axis axis1PhiExplicit{AxisBound, -45_degree * 30_mm, 45_degree * 30_mm, 13};
    auto grid2dCylPhiExplicit = grid1dCylPhi->extendTo2d(&axis1PhiExplicit);
    BOOST_REQUIRE(grid2dCylPhiExplicit);
    BOOST_CHECK_EQUAL(grid2dCylPhiExplicit->grid().axes().size(), 2);
    axis1Phi = grid2dCylPhiExplicit->grid().axes().front();
    axis2Phi = grid2dCylPhiExplicit->grid().axes().back();
    BOOST_CHECK_EQUAL(*axis1Phi, axis1PhiExplicit);
    BOOST_CHECK_EQUAL(*axis2Phi, axis2PhiExpected);

    auto& concrete4 =
        dynamic_cast<GridPortalLinkT<decltype(axis1PhiExplicit),
                                     decltype(axis2PhiExpected)>&>(
            *grid2dCylPhiExplicit);

    checkAllBins(concrete4);
  }

  BOOST_TEST_CONTEXT("2D") {
    auto cyl = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                    30_mm, 100_mm, 45_degree);

    // z bad, rphi bad
    BOOST_CHECK_THROW(GridPortalLink::make(cyl, Axis{AxisBound, 1, 2, 5},
                                           Axis{AxisBound, 3_mm, 4_mm, 5}),
                      std::invalid_argument);

    // z good, rphi bad
    BOOST_CHECK_THROW(GridPortalLink::make(cyl, Axis{AxisBound, 3_mm, 4_mm, 5},
                                           Axis{AxisBound, -100_mm, 100_m, 5}),
                      std::invalid_argument);

    // z bad, rphi good
    BOOST_CHECK_THROW(
        GridPortalLink::make(
            cyl, Axis{AxisBound, -45_degree * 30_mm, 45_degree * 30_mm, 5},
            Axis{AxisBound, -80_mm, 100_mm, 5}),
        std::invalid_argument);

    auto grid1 = GridPortalLink::make(
        cyl, Axis{AxisBound, -45_degree * 30_mm, 45_degree * 30_mm, 5},
        Axis{AxisBound, -100_mm, 100_mm, 5});

    auto cylFull = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                        30_mm, 100_mm);

    // Throws because non-closed axis
    BOOST_CHECK_THROW(GridPortalLink::make(cylFull,
                                           Axis{AxisBound, -180_degree * 30_mm,
                                                180_degree * 30_mm, 5},
                                           Axis{AxisBound, -100_mm, 100_mm, 5}),
                      std::invalid_argument);

    auto gridFull = GridPortalLink::make(
        cylFull, Axis{AxisClosed, -180_degree * 30_mm, 180_degree * 30_mm, 5},
        Axis{AxisBound, -100_mm, 100_mm, 5});

    BOOST_CHECK_EQUAL(gridFull->grid().axes().size(), 2);
    BOOST_CHECK_EQUAL(gridFull->grid().axes().size(), 2);
    Axis axisFullExpected{AxisClosed, -180_degree * 30_mm, 180_degree * 30_mm,
                          5};
    const auto& axisFull = *gridFull->grid().axes().front();
    BOOST_CHECK_EQUAL(axisFull, axisFullExpected);
  }
}

BOOST_AUTO_TEST_CASE(Disc) {
  using enum AxisType;
  using enum AxisBoundaryType;
  BOOST_TEST_CONTEXT("1D") {
    auto disc1 =
        Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm, 100_mm);

    // Volume for bin testing
    auto vol = std::make_shared<TrackingVolume>(
        Transform3::Identity(),
        std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

    BOOST_CHECK_THROW(GridPortalLink::make(disc1, BinningValue::binZ,
                                           Axis{AxisBound, 30_mm, 100_mm, 3}),
                      std::invalid_argument);

    // Check exception for full disc and non-closed phi axis
    BOOST_CHECK_THROW(
        GridPortalLink::make(disc1, BinningValue::binPhi,
                             Axis{AxisBound, -180_degree, 180_degree, 3}),
        std::invalid_argument);

    auto grid1 = GridPortalLink::make(disc1, BinningValue::binR,
                                      Axis{AxisBound, 30_mm, 100_mm, 3});
    BOOST_REQUIRE_NE(grid1, nullptr);
    BOOST_CHECK_EQUAL(grid1->grid().axes().size(), 1);
    const auto& axis = *grid1->grid().axes().front();
    Axis axis1Expected{AxisBound, 30_mm, 100_mm, 3};
    BOOST_CHECK_EQUAL(axis, axis1Expected);

    grid1->setVolume(vol.get());

    auto discPhi = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                    30_mm, 100_mm, 45_degree);

    // Check thet disc with phi sector does not accept closed axis
    BOOST_CHECK_THROW(
        GridPortalLink::make(discPhi, BinningValue::binPhi,
                             Axis{AxisClosed, -45_degree, 45_degree, 3}),
        std::invalid_argument);

    auto gridPhi =
        GridPortalLink::make(discPhi, BinningValue::binPhi,
                             Axis{AxisBound, -45_degree, 45_degree, 3});
    BOOST_REQUIRE_NE(gridPhi, nullptr);
    gridPhi->setVolume(vol.get());

    // Test exception on disc with non-zero average phi
    auto discNonZeroAverage = Surface::makeShared<DiscSurface>(
        Transform3::Identity(),
        std::make_shared<RadialBounds>(30_mm, 100_mm, 45_degree, 75_degree));
    BOOST_CHECK_THROW(
        GridPortalLink::make(discNonZeroAverage, BinningValue::binR,
                             Axis{AxisBound, 30_mm, 100_mm, 3}),
        std::invalid_argument);

    BOOST_CHECK_EQUAL(gridPhi->grid().axes().size(), 1);
    const auto& axisPhi = *gridPhi->grid().axes().front();
    Axis axisPhi1Expected{AxisBound, -45_degree, 45_degree, 3};
    BOOST_CHECK_EQUAL(axisPhi, axisPhi1Expected);

    auto checkAllBins = [&](const auto& grid) {
      visitBins(grid, [&](const TrackingVolume* content) {
        BOOST_CHECK_EQUAL(content, vol.get());
      });
    };

    checkAllBins(*grid1);
    checkAllBins(*gridPhi);

    // Test making 2D grids from the 1D ones
    auto grid2d = grid1->extendTo2d(nullptr);
    BOOST_REQUIRE(grid2d);
    BOOST_CHECK_EQUAL(grid2d->grid().axes().size(), 2);
    const auto* axis1 = grid2d->grid().axes().front();
    BOOST_CHECK_EQUAL(*axis1, axis1Expected);
    const auto* axis2 = grid2d->grid().axes().back();
    BOOST_CHECK_CLOSE(axis2->getMin(), -180_degree, 1e-6);
    BOOST_CHECK_CLOSE(axis2->getMax(), 180_degree, 1e-6);
    BOOST_CHECK_EQUAL(axis2->getNBins(), 1);
    BOOST_CHECK_EQUAL(axis2->getType(), AxisType::Equidistant);
    BOOST_CHECK_EQUAL(axis2->getBoundaryType(), AxisBoundaryType::Closed);

    checkAllBins(
        dynamic_cast<GridPortalLinkT<decltype(axisPhi1Expected),
                                     Axis<Equidistant, Closed>>&>(*grid2d));

    Axis axis2Explicit{AxisClosed, -180_degree, 180_degree, 3};
    auto grid2dExplicit = grid1->extendTo2d(&axis2Explicit);
    BOOST_REQUIRE(grid2dExplicit);
    BOOST_CHECK_EQUAL(grid2dExplicit->grid().axes().size(), 2);
    axis1 = grid2dExplicit->grid().axes().front();
    axis2 = grid2dExplicit->grid().axes().back();
    BOOST_CHECK_EQUAL(*axis1, axis1Expected);
    BOOST_CHECK_EQUAL(*axis2, axis2Explicit);

    checkAllBins(dynamic_cast<GridPortalLinkT<decltype(axisPhi1Expected),
                                              decltype(axis2Explicit)>&>(
        *grid2dExplicit));

    auto gridPhiBinnedInR = GridPortalLink::make(
        discPhi, BinningValue::binR, Axis{AxisBound, 30_mm, 100_mm, 3});
    gridPhiBinnedInR->setVolume(vol.get());
    auto grid2dPhiNonClosed = gridPhiBinnedInR->extendTo2d(nullptr);
    BOOST_REQUIRE(grid2dPhiNonClosed);
    BOOST_CHECK_EQUAL(grid2dPhiNonClosed->grid().axes().size(), 2);
    Axis gridPhiBinnedInRExpected{AxisBound, 30_mm, 100_mm, 3};
    BOOST_CHECK_EQUAL(*grid2dPhiNonClosed->grid().axes().front(),
                      gridPhiBinnedInRExpected);
    const auto* axisPhiNonClosed = grid2dPhiNonClosed->grid().axes().back();
    BOOST_CHECK_CLOSE(axisPhiNonClosed->getMin(), -45_degree, 1e-6);
    BOOST_CHECK_CLOSE(axisPhiNonClosed->getMax(), 45_degree, 1e-6);
    BOOST_CHECK_EQUAL(axisPhiNonClosed->getNBins(), 1);
    BOOST_CHECK_EQUAL(axisPhiNonClosed->getType(), AxisType::Equidistant);
    BOOST_CHECK_EQUAL(axisPhiNonClosed->getBoundaryType(),
                      AxisBoundaryType::Bound);

    checkAllBins(
        dynamic_cast<GridPortalLinkT<decltype(gridPhiBinnedInRExpected),
                                     Axis<Equidistant, Bound>>&>(
            *grid2dPhiNonClosed));

    Axis axisPhiNonClosedExplicit{AxisBound, -45_degree, 45_degree, 3};
    auto grid2dPhiNonClosedExplicit =
        gridPhiBinnedInR->extendTo2d(&axisPhiNonClosedExplicit);
    BOOST_REQUIRE(grid2dPhiNonClosedExplicit);
    BOOST_CHECK_EQUAL(grid2dPhiNonClosedExplicit->grid().axes().size(), 2);
    axisPhiNonClosed = grid2dPhiNonClosedExplicit->grid().axes().back();
    BOOST_CHECK_EQUAL(*axisPhiNonClosed, axisPhiNonClosedExplicit);
    BOOST_CHECK_EQUAL(*grid2dPhiNonClosedExplicit->grid().axes().front(),
                      gridPhiBinnedInRExpected);

    checkAllBins(
        dynamic_cast<GridPortalLinkT<decltype(gridPhiBinnedInRExpected),
                                     decltype(axisPhiNonClosedExplicit)>&>(
            *grid2dPhiNonClosedExplicit));

    auto grid2dPhi = gridPhi->extendTo2d(nullptr);
    BOOST_REQUIRE(grid2dPhi);
    BOOST_CHECK_EQUAL(grid2dPhi->grid().axes().size(), 2);
    Axis axis2dPhiExpected{AxisBound, 30_mm, 100_mm, 1};
    BOOST_CHECK_EQUAL(*grid2dPhi->grid().axes().front(), axis2dPhiExpected);
    BOOST_CHECK_EQUAL(*grid2dPhi->grid().axes().back(), axisPhi1Expected);

    checkAllBins(
        dynamic_cast<GridPortalLinkT<decltype(axis2dPhiExpected),
                                     decltype(axisPhi1Expected)>&>(*grid2dPhi));

    Axis axis2dPhiExplicit{AxisBound, 30_mm, 100_mm, 3};
    auto grid2dPhiExplicit = gridPhi->extendTo2d(&axis2dPhiExplicit);
    BOOST_REQUIRE(grid2dPhiExplicit);
    BOOST_CHECK_EQUAL(grid2dPhiExplicit->grid().axes().size(), 2);
    BOOST_CHECK_EQUAL(*grid2dPhiExplicit->grid().axes().front(),
                      axis2dPhiExplicit);
    BOOST_CHECK_EQUAL(*grid2dPhiExplicit->grid().axes().back(),
                      axisPhi1Expected);

    checkAllBins(dynamic_cast<GridPortalLinkT<decltype(axisPhi1Expected),
                                              decltype(axis2dPhiExplicit)>&>(
        *grid2dPhiExplicit));
  }

  BOOST_TEST_CONTEXT("2D") {
    auto discPhi = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                    30_mm, 100_mm, 45_degree);

    Axis rBad{AxisBound, 1, 2, 5};
    Axis rGood{AxisBound, 30_mm, 100_mm, 5};
    Axis phiBad{AxisBound, 1, 2, 5};
    Axis phiGood{AxisBound, -45_degree, 45_degree, 5};

    // r bad, phi bad
    BOOST_CHECK_THROW(GridPortalLink::make(discPhi, rBad, phiBad),
                      std::invalid_argument);
    // r bad, phi good
    BOOST_CHECK_THROW(GridPortalLink::make(discPhi, rBad, phiGood),
                      std::invalid_argument);
    // r good, phi bad
    BOOST_CHECK_THROW(GridPortalLink::make(discPhi, rGood, phiBad),
                      std::invalid_argument);
    // r good phi good
    auto grid = GridPortalLink::make(discPhi, rGood, phiGood);
    BOOST_REQUIRE_NE(grid, nullptr);
  }
}

BOOST_AUTO_TEST_CASE(Plane) {
  // @TODO: Add plane tests
}

BOOST_AUTO_TEST_CASE(FromTrivial) {
  BOOST_TEST_CONTEXT("Cylinder") {
    auto cyl = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                    30_mm, 100_mm);

    auto vol = std::make_shared<TrackingVolume>(
        Transform3::Identity(),
        std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

    auto trivial = std::make_unique<TrivialPortalLink>(cyl, *vol);
    BOOST_REQUIRE(trivial);

    BOOST_CHECK_EQUAL(trivial->resolveVolume(gctx, Vector2{1, 2}).value(),
                      vol.get());

    auto gridZ = trivial->makeGrid(BinningValue::binZ);
    BOOST_REQUIRE(gridZ);

    BOOST_CHECK_EQUAL(gridZ->grid().axes().size(), 1);
    BOOST_CHECK_EQUAL(gridZ->surface().bounds(), cyl->bounds());
    Axis axisZExpected{AxisBound, -100_mm, 100_mm, 1};
    BOOST_CHECK_EQUAL(*gridZ->grid().axes().front(), axisZExpected);

    BOOST_CHECK_EQUAL(
        gridZ->resolveVolume(gctx, Vector2{20_degree * 30_mm, 90_mm}).value(),
        vol.get());

    auto gridRPhi = trivial->makeGrid(BinningValue::binRPhi);
    BOOST_REQUIRE(gridRPhi);

    BOOST_CHECK_EQUAL(gridRPhi->grid().axes().size(), 1);
    BOOST_CHECK_EQUAL(gridRPhi->surface().bounds(), cyl->bounds());
    Axis axisRPhiExpected{AxisClosed, -M_PI * 30_mm, M_PI * 30_mm, 1};
    BOOST_CHECK_EQUAL(*gridRPhi->grid().axes().front(), axisRPhiExpected);

    auto cylPhi = Surface::makeShared<CylinderSurface>(
        Transform3::Identity(), 30_mm, 100_mm, 30_degree);

    auto trivialPhi = std::make_unique<TrivialPortalLink>(cylPhi, *vol);
    BOOST_REQUIRE(trivialPhi);

    BOOST_CHECK_EQUAL(trivialPhi->resolveVolume(gctx, Vector2{1, 2}).value(),
                      vol.get());

    auto gridRPhiSector = trivialPhi->makeGrid(BinningValue::binRPhi);
    BOOST_REQUIRE(gridRPhiSector);

    BOOST_CHECK_EQUAL(
        gridRPhiSector->resolveVolume(gctx, Vector2{20_degree * 30_mm, 90_mm})
            .value(),
        vol.get());

    BOOST_CHECK_EQUAL(gridRPhiSector->grid().axes().size(), 1);
    Axis axisRPhiSectorExpected{AxisBound, -30_degree * 30_mm,
                                30_degree * 30_mm, 1};
    BOOST_CHECK_EQUAL(*gridRPhiSector->grid().axes().front(),
                      axisRPhiSectorExpected);
  }

  BOOST_TEST_CONTEXT("Disc") {
    auto disc =
        Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm, 100_mm);

    auto vol = std::make_shared<TrackingVolume>(
        Transform3::Identity(),
        std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

    auto trivial = std::make_unique<TrivialPortalLink>(disc, *vol);
    BOOST_REQUIRE(trivial);

    // Doesn't matter which position
    BOOST_CHECK_EQUAL(trivial->resolveVolume(gctx, Vector2{1, 2}).value(),
                      vol.get());

    auto gridR = trivial->makeGrid(BinningValue::binR);
    BOOST_REQUIRE(gridR);

    BOOST_CHECK_EQUAL(gridR->grid().axes().size(), 1);
    BOOST_CHECK_EQUAL(gridR->surface().bounds(), disc->bounds());
    Axis axisRExpected{AxisBound, 30_mm, 100_mm, 1};
    BOOST_CHECK_EQUAL(*gridR->grid().axes().front(), axisRExpected);

    BOOST_CHECK_EQUAL(
        gridR->resolveVolume(gctx, Vector2{90_mm, 10_degree}).value(),
        vol.get());

    auto gridPhi = trivial->makeGrid(BinningValue::binPhi);
    BOOST_REQUIRE(gridPhi);

    BOOST_CHECK_EQUAL(gridPhi->grid().axes().size(), 1);
    BOOST_CHECK_EQUAL(gridPhi->surface().bounds(), disc->bounds());
    Axis axisPhiExpected{AxisClosed, -M_PI, M_PI, 1};
    BOOST_CHECK_EQUAL(*gridPhi->grid().axes().front(), axisPhiExpected);

    BOOST_CHECK_EQUAL(
        gridPhi->resolveVolume(gctx, Vector2{90_mm, 10_degree}).value(),
        vol.get());
  }
}

BOOST_AUTO_TEST_SUITE_END()  // GridConstruction

BOOST_AUTO_TEST_SUITE(GridMerging)

BOOST_AUTO_TEST_SUITE(Merging1dCylinder)

BOOST_AUTO_TEST_SUITE(ZDirection)

BOOST_AUTO_TEST_CASE(ColinearMerge) {
  auto vol1 = makeDummyVolume();
  auto vol2 = makeDummyVolume();

  auto cyl = Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30_mm,
                                                  100_mm);

  auto grid1dCyl = GridPortalLink::make(cyl, BinningValue::binZ,
                                        Axis{AxisBound, -100_mm, 100_mm, 10});
  grid1dCyl->setVolume(vol1.get());

  // Another cylinder, shifted in z
  auto cyl2 = Surface::makeShared<CylinderSurface>(
      Transform3{Translation3{Vector3::UnitZ() * 150_mm}}, 30_mm, 50_mm);

  auto grid1dCyl2 = GridPortalLink::make(cyl2, BinningValue::binZ,
                                         Axis{AxisBound, -50_mm, 50_mm, 5});

  grid1dCyl2->setVolume(vol2.get());

  // Completely invalid
  BOOST_CHECK_THROW(GridPortalLink::merge(*grid1dCyl, *grid1dCyl2,
                                          BinningValue::binPhi, *logger),
                    AssertionFailureException);
  // Invalid direction, as the cylinders are shifted in z, and can't be merged
  // in r x phi
  BOOST_CHECK_THROW(GridPortalLink::merge(*grid1dCyl, *grid1dCyl2,
                                          BinningValue::binRPhi, *logger),
                    SurfaceMergingException);

  BOOST_TEST_CONTEXT("Consistent equidistant") {
    auto mergedPtr = GridPortalLink::merge(*grid1dCyl, *grid1dCyl2,
                                           BinningValue::binZ, *logger);
    BOOST_REQUIRE_NE(mergedPtr, nullptr);
    const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
    BOOST_REQUIRE_NE(merged, nullptr);
    BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
    const auto& axis = *merged->grid().axes().front();
    BOOST_CHECK_EQUAL(axis.getMin(), -150_mm);
    BOOST_CHECK_EQUAL(axis.getMax(), 150_mm);
    BOOST_CHECK_EQUAL(axis.getNBins(), 15);
    BOOST_CHECK_EQUAL(axis.getType(), AxisType::Equidistant);
    BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Bound);
  }

  BOOST_TEST_CONTEXT("Inconsistent equidistant") {
    auto grid1dCyl2BinWidthChanged = GridPortalLink::make(
        cyl2, BinningValue::binZ, Axis{AxisBound, -50_mm, 50_mm, 6});

    auto mergedPtr = GridPortalLink::merge(
        *grid1dCyl, *grid1dCyl2BinWidthChanged, BinningValue::binZ, *logger);
    BOOST_REQUIRE_NE(mergedPtr, nullptr);
    const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
    BOOST_REQUIRE_NE(merged, nullptr);
    BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
    const auto& axis = *merged->grid().axes().front();
    BOOST_CHECK_EQUAL(axis.getMin(), -150_mm);
    BOOST_CHECK_EQUAL(axis.getMax(), 150_mm);
    BOOST_CHECK_EQUAL(axis.getNBins(), 16);
    BOOST_CHECK_EQUAL(axis.getType(), AxisType::Variable);
    BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Bound);
  }

  BOOST_TEST_CONTEXT("Right Variable") {
    auto gridLeft = GridPortalLink::make(cyl, BinningValue::binZ,
                                         Axis{AxisBound, -100_mm, 100_mm, 10});

    auto gridRight =
        GridPortalLink::make(cyl2, BinningValue::binZ,
                             Axis{AxisBound, {-50_mm, -10_mm, 10_mm, 50_mm}});

    auto mergedPtr = GridPortalLink::merge(*gridLeft, *gridRight,
                                           BinningValue::binZ, *logger);
    BOOST_REQUIRE_NE(mergedPtr, nullptr);
    const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
    BOOST_REQUIRE_NE(merged, nullptr);
    BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
    const auto& axis = *merged->grid().axes().front();
    BOOST_CHECK_EQUAL(axis.getMin(), -150_mm);
    BOOST_CHECK_EQUAL(axis.getMax(), 150_mm);
    BOOST_CHECK_EQUAL(axis.getNBins(), 13);
    BOOST_CHECK_EQUAL(axis.getType(), AxisType::Variable);
    BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Bound);
  }

  BOOST_TEST_CONTEXT("Left Variable") {
    auto gridLeft =
        GridPortalLink::make(cyl, BinningValue::binZ,
                             Axis{AxisBound, {-100_mm, -80_mm, 10_mm, 100_mm}});

    auto gridRight = GridPortalLink::make(cyl2, BinningValue::binZ,
                                          Axis{AxisBound, -50_mm, 50_mm, 8});

    auto mergedPtr = GridPortalLink::merge(*gridLeft, *gridRight,
                                           BinningValue::binZ, *logger);
    BOOST_REQUIRE_NE(mergedPtr, nullptr);
    const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
    BOOST_REQUIRE_NE(merged, nullptr);
    BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
    const auto& axis = *merged->grid().axes().front();
    BOOST_CHECK_EQUAL(axis.getMin(), -150_mm);
    BOOST_CHECK_EQUAL(axis.getMax(), 150_mm);
    BOOST_CHECK_EQUAL(axis.getNBins(), 11);
    BOOST_CHECK_EQUAL(axis.getType(), AxisType::Variable);
    BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Bound);
  }

  BOOST_TEST_CONTEXT("Both Variable") {
    auto gridLeft =
        GridPortalLink::make(cyl, BinningValue::binZ,
                             Axis{AxisBound, {-100_mm, -80_mm, 10_mm, 100_mm}});

    auto gridRight =
        GridPortalLink::make(cyl2, BinningValue::binZ,
                             Axis{AxisBound, {-50_mm, -10_mm, 10_mm, 50_mm}});

    auto mergedPtr = GridPortalLink::merge(*gridLeft, *gridRight,
                                           BinningValue::binZ, *logger);
    BOOST_REQUIRE_NE(mergedPtr, nullptr);
    const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
    BOOST_REQUIRE_NE(merged, nullptr);
    BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
    const auto& axis = *merged->grid().axes().front();
    BOOST_CHECK_EQUAL(axis.getMin(), -150_mm);
    BOOST_CHECK_EQUAL(axis.getMax(), 150_mm);
    BOOST_CHECK_EQUAL(axis.getNBins(), 6);
    BOOST_CHECK_EQUAL(axis.getType(), AxisType::Variable);
    BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Bound);
  }

  BOOST_TEST_CONTEXT("Non bound axis") {
    std::unique_ptr gridLeft =
        GridPortalLink::make(cyl, BinningValue::binZ,
                             Axis{AxisBound, {-100_mm, -80_mm, 10_mm, 100_mm}});
    std::unique_ptr gridRightClosed =
        GridPortalLink::make(cyl2, BinningValue::binZ,
                             Axis{AxisClosed, {-50_mm, -10_mm, 10_mm, 50_mm}});
    std::unique_ptr gridRightOpen =
        GridPortalLink::make(cyl2, BinningValue::binZ,
                             Axis{AxisOpen, {-50_mm, -10_mm, 10_mm, 50_mm}});

    auto compositeLR = PortalLinkBase::merge(
        copy(gridLeft), copy(gridRightClosed), BinningValue::binZ, *logger);
    BOOST_CHECK_NE(dynamic_cast<const CompositePortalLink*>(compositeLR.get()),
                   nullptr);
    auto compositeRL = PortalLinkBase::merge(
        copy(gridLeft), copy(gridRightOpen), BinningValue::binZ, *logger);
    BOOST_CHECK_NE(dynamic_cast<const CompositePortalLink*>(compositeRL.get()),
                   nullptr);
  }
}

BOOST_AUTO_TEST_CASE(ParallelMerge) {
  // Merge in z direction with phi sectors
  auto cyl1 = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                   30_mm, 100_mm, 35_degree);

  auto grid1 = GridPortalLink::make(
      cyl1, BinningValue::binRPhi,

      Axis{AxisBound, -35_degree * 30_mm, 35_degree * 30_mm, 3});
  auto cyl2 = Surface::makeShared<CylinderSurface>(
      Transform3{Translation3{Vector3::UnitZ() * 150_mm}}, 30_mm, 50_mm,
      35_degree);

  auto grid2 = GridPortalLink::make(
      cyl2, BinningValue::binRPhi,
      Axis{AxisBound, -35_degree * 30_mm, 35_degree * 30_mm, 3});

  auto merged12Ptr =
      GridPortalLink::merge(*grid1, *grid2, BinningValue::binZ, *logger);
  BOOST_REQUIRE_NE(merged12Ptr, nullptr);
  auto merged12 = dynamic_cast<const GridPortalLink*>(merged12Ptr.get());
  BOOST_REQUIRE_NE(merged12, nullptr);

  BOOST_REQUIRE_EQUAL(merged12->grid().axes().size(), 2);

  const auto& axis1 = *merged12->grid().axes().front();
  const auto& axis2 = *merged12->grid().axes().back();
  Axis axis1Expected{AxisBound, -35_degree * 30_mm, 35_degree * 30_mm, 3};
  BOOST_CHECK_EQUAL(axis1, axis1Expected);
  Axis axis2Expected{AxisBound, {-150_mm, 50_mm, 150_mm}};
  BOOST_CHECK_EQUAL(axis2, axis2Expected);
}

BOOST_AUTO_TEST_SUITE_END()  // ZDirection

BOOST_AUTO_TEST_SUITE(RPhiDirection)

BOOST_AUTO_TEST_CASE(ColinearMerge) {
  auto cyl = Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30_mm,
                                                  100_mm);
  BOOST_CHECK_THROW(GridPortalLink::make(cyl, BinningValue::binRPhi,
                                         Axis{AxisBound, 0, 5, 5}),
                    std::invalid_argument);

  auto cylNonZeroAverage = Surface::makeShared<CylinderSurface>(
      Transform3::Identity(), 30_mm, 100_mm, 20_degree, 45_degree);

  BOOST_CHECK_THROW(
      GridPortalLink::make(
          cylNonZeroAverage, BinningValue::binRPhi,
          Axis{AxisBound, -20_degree * 30_mm, 20_degree * 30_mm, 5}),
      std::invalid_argument);

  BOOST_TEST_CONTEXT("Colinear merge in rPhi") {
    auto cylPhi1 = Surface::makeShared<CylinderSurface>(
        Transform3::Identity() * AngleAxis3(45_degree, Vector3::UnitZ()), 30_mm,
        100_mm, 20_degree, 0_degree);

    auto cylPhi2 = Surface::makeShared<CylinderSurface>(
        Transform3::Identity() * AngleAxis3(105_degree, Vector3::UnitZ()),
        30_mm, 100_mm, 40_degree, 0_degree);

    auto portalPhi1 = GridPortalLink::make(
        cylPhi1, BinningValue::binRPhi,
        Axis{AxisBound, -20_degree * 30_mm, 20_degree * 30_mm, 5});

    auto portalPhi2 = GridPortalLink::make(
        cylPhi2, BinningValue::binRPhi,
        Axis{AxisBound, -40_degree * 30_mm, 40_degree * 30_mm, 10});

    auto cylPhi3 = Surface::makeShared<CylinderSurface>(
        Transform3::Identity() * AngleAxis3(45_degree, Vector3::UnitZ()), 30_mm,
        100_mm, 90_degree, 0_degree);

    auto cylPhi4 = Surface::makeShared<CylinderSurface>(
        Transform3::Identity() * AngleAxis3(-135_degree, Vector3::UnitZ()),
        30_mm, 100_mm, 90_degree, 0_degree);

    auto portalPhi3 = GridPortalLink::make(
        cylPhi3, BinningValue::binRPhi,
        Axis{AxisBound, -90_degree * 30_mm, 90_degree * 30_mm, 2});

    auto portalPhi4 = GridPortalLink::make(
        cylPhi4, BinningValue::binRPhi,
        Axis{AxisBound, -90_degree * 30_mm, 90_degree * 30_mm, 2});

    BOOST_TEST_CONTEXT("Consistent equidistant") {
      auto portalMerged = GridPortalLink::merge(*portalPhi1, *portalPhi2,
                                                BinningValue::binRPhi, *logger);
      BOOST_REQUIRE_NE(portalMerged, nullptr);

      const auto* merged =
          dynamic_cast<const GridPortalLink*>(portalMerged.get());
      BOOST_REQUIRE_NE(merged, nullptr);
      BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
      const auto& axis = *merged->grid().axes().front();
      BOOST_CHECK_CLOSE(axis.getMin(), -60_degree * 30_mm, 1e-9);
      BOOST_CHECK_CLOSE(axis.getMax(), 60_degree * 30_mm, 1e-9);
      BOOST_CHECK_EQUAL(axis.getNBins(), 15);
      BOOST_CHECK_EQUAL(axis.getType(), AxisType::Equidistant);
      BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Bound);

      // Test that if you merge half-circles, we get a closed axis
      auto portalMerged34 = GridPortalLink::merge(
          *portalPhi3, *portalPhi4, BinningValue::binRPhi, *logger);
      BOOST_REQUIRE_NE(portalMerged34, nullptr);

      const auto* merged34 =
          dynamic_cast<const GridPortalLink*>(portalMerged34.get());
      BOOST_REQUIRE_NE(merged34, nullptr);
      BOOST_CHECK_EQUAL(merged34->grid().axes().size(), 1);
      const auto& axis34 = *merged34->grid().axes().front();
      BOOST_CHECK_CLOSE(axis34.getMin(), -180_degree * 30_mm, 1e-9);
      BOOST_CHECK_CLOSE(axis34.getMax(), 180_degree * 30_mm, 1e-9);
      BOOST_CHECK_EQUAL(axis34.getNBins(), 4);
      BOOST_CHECK_EQUAL(axis34.getType(), AxisType::Equidistant);
      BOOST_CHECK_EQUAL(axis34.getBoundaryType(), AxisBoundaryType::Closed);
    }

    BOOST_TEST_CONTEXT("Inconsistent equidistant") {
      auto portalPhi2Mod = GridPortalLink::make(
          cylPhi2, BinningValue::binRPhi,
          Axis{AxisBound, -40_degree * 30_mm, 40_degree * 30_mm, 3});

      auto portalMergedMod = GridPortalLink::merge(
          *portalPhi1, *portalPhi2Mod, BinningValue::binRPhi, *logger);
      BOOST_REQUIRE_NE(portalMergedMod, nullptr);

      const auto* merged12 =
          dynamic_cast<const GridPortalLink*>(portalMergedMod.get());
      BOOST_REQUIRE_NE(merged12, nullptr);
      BOOST_CHECK_EQUAL(merged12->grid().axes().size(), 1);
      const auto& axis12 = *merged12->grid().axes().front();
      BOOST_CHECK_CLOSE(axis12.getMin(), -60_degree * 30_mm, 1e-9);
      BOOST_CHECK_CLOSE(axis12.getMax(), 60_degree * 30_mm, 1e-9);
      BOOST_CHECK_EQUAL(axis12.getNBins(), 8);
      BOOST_CHECK_EQUAL(axis12.getType(), AxisType::Variable);
      BOOST_CHECK_EQUAL(axis12.getBoundaryType(), AxisBoundaryType::Bound);

      std::vector<ActsScalar> expected12 = {-31.4159, -17.4533, -3.49066,
                                            10.472,   14.6608,  18.8496,
                                            23.0383,  27.2271,  31.4159};
      CHECK_CLOSE_OR_SMALL(axis12.getBinEdges(), expected12, 1e-4, 10e-10);

      auto portalPhi4Mod = GridPortalLink::make(
          cylPhi4, BinningValue::binRPhi,
          Axis{AxisBound, -90_degree * 30_mm, 90_degree * 30_mm, 1});

      auto portalMerged34 = GridPortalLink::merge(
          *portalPhi3, *portalPhi4Mod, BinningValue::binRPhi, *logger);
      BOOST_REQUIRE_NE(portalMerged34, nullptr);

      const auto* merged34 =
          dynamic_cast<const GridPortalLink*>(portalMerged34.get());
      BOOST_REQUIRE_NE(merged34, nullptr);
      BOOST_CHECK_EQUAL(merged34->grid().axes().size(), 1);
      const auto& axis34 = *merged34->grid().axes().front();
      BOOST_CHECK_CLOSE(axis34.getMin(), -180_degree * 30_mm, 1e-9);
      BOOST_CHECK_CLOSE(axis34.getMax(), 180_degree * 30_mm, 1e-9);
      BOOST_CHECK_EQUAL(axis34.getNBins(), 3);
      BOOST_CHECK_EQUAL(axis34.getType(), AxisType::Variable);
      BOOST_CHECK_EQUAL(axis34.getBoundaryType(), AxisBoundaryType::Closed);

      // Caution: for full-azimuth cases, the ordering is preserved, you get
      // in what you get out. -> this can flip
      std::vector<ActsScalar> expected34 = {-94.2478, -47.1239, 0, 94.2478};
      CHECK_CLOSE_OR_SMALL(axis34.getBinEdges(), expected34, 1e-4, 10e-10);
    }

    BOOST_TEST_CONTEXT("Left variable") {
      BOOST_TEST_CONTEXT("Non-closed") {
        auto gridLeft =
            GridPortalLink::make(cylPhi1, BinningValue::binRPhi,
                                 Axis{AxisBound,
                                      {-20_degree * 30_mm, -10_degree * 30_mm,
                                       10_degree * 30_mm, 20_degree * 30_mm}});
        auto gridRight = GridPortalLink::make(
            cylPhi2, BinningValue::binRPhi,
            Axis{AxisBound, -40_degree * 30_mm, 40_degree * 30_mm, 3});

        auto mergedPtr = GridPortalLink::merge(*gridLeft, *gridRight,
                                               BinningValue::binRPhi, *logger);
        BOOST_REQUIRE_NE(mergedPtr, nullptr);

        const auto* merged =
            dynamic_cast<const GridPortalLink*>(mergedPtr.get());
        BOOST_REQUIRE_NE(merged, nullptr);
        const auto& axis = *merged->grid().axes().front();
        BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
        BOOST_CHECK_CLOSE(axis.getMin(), -60_degree * 30_mm, 1e-9);
        BOOST_CHECK_CLOSE(axis.getMax(), 60_degree * 30_mm, 1e-9);
        BOOST_CHECK_EQUAL(axis.getNBins(), 6);
        BOOST_CHECK_EQUAL(axis.getType(), AxisType::Variable);
        BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Bound);

        std::vector<ActsScalar> expected = {
            -31.4159, -17.4533, -3.49066, 10.472, 15.708, 26.1799, 31.4159};
        CHECK_CLOSE_OR_SMALL(axis.getBinEdges(), expected, 1e-4, 10e-10);
      }

      BOOST_TEST_CONTEXT("Closed") {
        auto gridLeft = GridPortalLink::make(
            cylPhi4, BinningValue::binRPhi,
            Axis{AxisBound,
                 {-90_degree * 30_mm, 25_degree * 30_mm, 90_degree * 30_mm}});

        auto gridRight = GridPortalLink::make(
            cylPhi3, BinningValue::binRPhi,
            Axis{AxisBound, -90_degree * 30_mm, 90_degree * 30_mm, 3});

        auto mergedPtr = GridPortalLink::merge(*gridLeft, *gridRight,
                                               BinningValue::binRPhi, *logger);
        BOOST_REQUIRE_NE(mergedPtr, nullptr);

        const auto* merged =
            dynamic_cast<const GridPortalLink*>(mergedPtr.get());
        BOOST_REQUIRE_NE(merged, nullptr);
        BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
        const auto& axis = *merged->grid().axes().front();
        BOOST_CHECK_CLOSE(axis.getMin(), -180_degree * 30_mm, 1e-9);
        BOOST_CHECK_CLOSE(axis.getMax(), 180_degree * 30_mm, 1e-9);
        BOOST_CHECK_EQUAL(axis.getNBins(), 5);
        BOOST_CHECK_EQUAL(axis.getType(), AxisType::Variable);
        BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Closed);

        // Caution: for full-azimuth cases, the ordering is preserved, you get
        // in what you get out. -> this can flip
        std::vector<ActsScalar> expected = {-94.2478, -34.0339, 0,
                                            31.4159,  62.8319,  94.2478};
        CHECK_CLOSE_OR_SMALL(axis.getBinEdges(), expected, 1e-4, 10e-10);
      }
    }

    BOOST_TEST_CONTEXT("Right variable") {
      BOOST_TEST_CONTEXT("Non-closed") {
        auto gridLeft = GridPortalLink::make(
            cylPhi1, BinningValue::binRPhi,
            Axis{AxisBound, -20_degree * 30_mm, 20_degree * 30_mm, 3});
        auto gridRight =
            GridPortalLink::make(cylPhi2, BinningValue::binRPhi,
                                 Axis{AxisBound,
                                      {-40_degree * 30_mm, -10_degree * 30_mm,
                                       10_degree * 30_mm, 40_degree * 30_mm}});

        auto mergedPtr = GridPortalLink::merge(*gridLeft, *gridRight,
                                               BinningValue::binRPhi, *logger);
        BOOST_REQUIRE_NE(mergedPtr, nullptr);

        const auto* merged =
            dynamic_cast<const GridPortalLink*>(mergedPtr.get());
        BOOST_REQUIRE_NE(merged, nullptr);
        const auto& axis = *merged->grid().axes().front();
        BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
        BOOST_CHECK_CLOSE(axis.getMin(), -60_degree * 30_mm, 1e-9);
        BOOST_CHECK_CLOSE(axis.getMax(), 60_degree * 30_mm, 1e-9);
        BOOST_CHECK_EQUAL(axis.getNBins(), 6);
        BOOST_CHECK_EQUAL(axis.getType(), AxisType::Variable);
        BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Bound);

        std::vector<ActsScalar> expected = {-31.4159, -15.708, -5.23599, 10.472,
                                            17.4533,  24.4346, 31.4159};
        CHECK_CLOSE_OR_SMALL(axis.getBinEdges(), expected, 1e-4, 10e-10);
      }

      BOOST_TEST_CONTEXT("Closed") {
        auto gridLeft = GridPortalLink::make(
            cylPhi4, BinningValue::binRPhi,
            Axis{AxisBound, -90_degree * 30_mm, 90_degree * 30_mm, 3});

        auto gridRight = GridPortalLink::make(
            cylPhi3, BinningValue::binRPhi,
            Axis{AxisBound,
                 {-90_degree * 30_mm, 25_degree * 30_mm, 90_degree * 30_mm}});

        auto mergedPtr = GridPortalLink::merge(*gridLeft, *gridRight,
                                               BinningValue::binRPhi, *logger);
        BOOST_REQUIRE_NE(mergedPtr, nullptr);

        const auto* merged =
            dynamic_cast<const GridPortalLink*>(mergedPtr.get());
        BOOST_REQUIRE_NE(merged, nullptr);
        BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
        const auto& axis = *merged->grid().axes().front();
        BOOST_CHECK_CLOSE(axis.getMin(), -180_degree * 30_mm, 1e-9);
        BOOST_CHECK_CLOSE(axis.getMax(), 180_degree * 30_mm, 1e-9);
        BOOST_CHECK_EQUAL(axis.getNBins(), 5);
        BOOST_CHECK_EQUAL(axis.getType(), AxisType::Variable);
        BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Closed);

        // Caution: for full-azimuth cases, the ordering is preserved, you get
        // in what you get out. -> this can flip
        std::vector<ActsScalar> expected = {-94.2478, -62.8319, -31.4159,
                                            0,        60.2139,  94.2478};
        CHECK_CLOSE_OR_SMALL(axis.getBinEdges(), expected, 1e-4, 10e-10);
      }
    }

    BOOST_TEST_CONTEXT("Both variable") {
      BOOST_TEST_CONTEXT("Non-closed") {
        auto gridLeft =
            GridPortalLink::make(cylPhi1, BinningValue::binRPhi,
                                 Axis{AxisBound,
                                      {-20_degree * 30_mm, -10_degree * 30_mm,
                                       10_degree * 30_mm, 20_degree * 30_mm}});
        auto gridRight = GridPortalLink::make(
            cylPhi2, BinningValue::binRPhi,
            Axis{AxisBound,
                 {-40_degree * 30_mm, -5_degree * 30_mm, 40_degree * 30_mm}});

        auto mergedPtr = GridPortalLink::merge(*gridLeft, *gridRight,
                                               BinningValue::binRPhi, *logger);
        BOOST_REQUIRE_NE(mergedPtr, nullptr);

        const auto* merged =
            dynamic_cast<const GridPortalLink*>(mergedPtr.get());
        BOOST_REQUIRE_NE(merged, nullptr);
        const auto& axis = *merged->grid().axes().front();
        BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
        BOOST_CHECK_CLOSE(axis.getMin(), -60_degree * 30_mm, 1e-9);
        BOOST_CHECK_CLOSE(axis.getMax(), 60_degree * 30_mm, 1e-9);
        BOOST_CHECK_EQUAL(axis.getNBins(), 5);
        BOOST_CHECK_EQUAL(axis.getType(), AxisType::Variable);
        BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Bound);

        std::vector<ActsScalar> expected = {-31.4159, -13.09,  10.472,
                                            15.708,   26.1799, 31.4159};
        CHECK_CLOSE_OR_SMALL(axis.getBinEdges(), expected, 1e-4, 10e-10);
      }

      BOOST_TEST_CONTEXT("Closed") {
        auto gridLeft = GridPortalLink::make(
            cylPhi4, BinningValue::binRPhi,
            Axis{AxisBound,
                 {-90_degree * 30_mm, 25_degree * 30_mm, 90_degree * 30_mm}});

        auto gridRight =
            GridPortalLink::make(cylPhi3, BinningValue::binRPhi,
                                 Axis{AxisBound,
                                      {-90_degree * 30_mm, -10_degree * 30_mm,
                                       10_degree * 30_mm, 90_degree * 30_mm}});

        auto mergedPtr = GridPortalLink::merge(*gridLeft, *gridRight,
                                               BinningValue::binRPhi, *logger);
        BOOST_REQUIRE_NE(mergedPtr, nullptr);

        const auto* merged =
            dynamic_cast<const GridPortalLink*>(mergedPtr.get());
        BOOST_REQUIRE_NE(merged, nullptr);
        BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
        const auto& axis = *merged->grid().axes().front();
        BOOST_CHECK_CLOSE(axis.getMin(), -180_degree * 30_mm, 1e-9);
        BOOST_CHECK_CLOSE(axis.getMax(), 180_degree * 30_mm, 1e-9);
        BOOST_CHECK_EQUAL(axis.getNBins(), 5);
        BOOST_CHECK_EQUAL(axis.getType(), AxisType::Variable);
        BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Closed);

        // Caution: for full-azimuth cases, the ordering is preserved, you get
        // in what you get out. -> this can flip
        std::vector<ActsScalar> expected = {-94.2478, -34.0339, 0,
                                            41.8879,  52.3599,  94.2478};
        CHECK_CLOSE_OR_SMALL(axis.getBinEdges(), expected, 1e-4, 10e-10);
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(ParallelMerge) {
  // Merge in phi direction with z binning
  auto cylPhi1 = Surface::makeShared<CylinderSurface>(
      Transform3::Identity() * AngleAxis3(45_degree, Vector3::UnitZ()), 30_mm,
      100_mm, 20_degree, 0_degree);

  auto cylPhi2 = Surface::makeShared<CylinderSurface>(
      Transform3::Identity() * AngleAxis3(85_degree, Vector3::UnitZ()), 30_mm,
      100_mm, 20_degree, 0_degree);

  auto portalPhi1 = GridPortalLink::make(cylPhi1, BinningValue::binZ,
                                         Axis{AxisBound, -100_mm, 100_mm, 5});

  auto portalPhi2 = GridPortalLink::make(cylPhi2, BinningValue::binZ,
                                         Axis{AxisBound, -100_mm, 100_mm, 5});

  auto merged12Ptr = GridPortalLink::merge(*portalPhi1, *portalPhi2,
                                           BinningValue::binRPhi, *logger);
  BOOST_REQUIRE_NE(merged12Ptr, nullptr);
  auto merged12 = dynamic_cast<const GridPortalLink*>(merged12Ptr.get());
  BOOST_REQUIRE_NE(merged12, nullptr);

  const auto& axis1 = *merged12->grid().axes().front();
  const auto& axis2 = *merged12->grid().axes().back();
  // Phi sectors were same size, should give equidistant binning
  Axis axis1Expected{AxisBound, -40_degree * 30_mm, 40_degree * 30_mm, 2};
  BOOST_CHECK_EQUAL(axis1, axis1Expected);
  Axis axis2Expected{AxisBound, -100_mm, 100_mm, 5};
  BOOST_CHECK_EQUAL(axis2, axis2Expected);
}

BOOST_AUTO_TEST_SUITE_END()  // RPhiDirection

BOOST_AUTO_TEST_SUITE_END()  // Merging1dCylinder

BOOST_AUTO_TEST_SUITE(Merging2dCylinder)

BOOST_AUTO_TEST_CASE(ZDirection) {
  BOOST_TEST_CONTEXT("Phi sector") {
    auto cyl1 = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                     30_mm, 100_mm, 45_degree);

    // z good, rphi good
    auto grid1 = GridPortalLink::make(
        cyl1, Axis{AxisBound, -45_degree * 30_mm, 45_degree * 30_mm, 5},
        Axis{AxisBound, -100_mm, 100_mm, 5});

    auto trf2 = Transform3{Translation3{Vector3::UnitZ() * 150_mm}};
    auto cyl2 =
        Surface::makeShared<CylinderSurface>(trf2, 30_mm, 50_mm, 45_degree);

    // Second grid portal with compatible phi binning
    auto grid2 = GridPortalLink::make(
        cyl2, Axis{AxisBound, -45_degree * 30_mm, 45_degree * 30_mm, 5},
        Axis{AxisBound, -50_mm, 50_mm, 5});

    // We're merging in z direction, so the phi binnings need to be the same

    auto mergedPtr =
        GridPortalLink::merge(*grid1, *grid2, BinningValue::binZ, *logger);
    const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
    BOOST_REQUIRE_NE(mergedPtr, nullptr);

    const auto& axis1 = *merged->grid().axes().front();
    const auto& axis2 = *merged->grid().axes().back();

    BOOST_CHECK_EQUAL(axis1.getMin(), -45_degree * 30_mm);
    BOOST_CHECK_EQUAL(axis1.getMax(), 45_degree * 30_mm);
    BOOST_CHECK_EQUAL(axis1.getNBins(), 5);
    BOOST_CHECK_EQUAL(axis1.getType(), AxisType::Equidistant);
    BOOST_CHECK_EQUAL(axis1.getBoundaryType(), AxisBoundaryType::Bound);

    BOOST_CHECK_EQUAL(axis2.getMin(), -150_mm);
    BOOST_CHECK_EQUAL(axis2.getMax(), 150_mm);
    BOOST_CHECK_EQUAL(axis2.getNBins(), 10);
    BOOST_CHECK_EQUAL(axis2.getType(), AxisType::Variable);
    BOOST_CHECK_EQUAL(axis2.getBoundaryType(), AxisBoundaryType::Bound);

    auto grid3 = GridPortalLink::make(
        cyl2, Axis{AxisBound, -45_degree * 30_mm, 45_degree * 30_mm, 3},
        Axis{AxisBound, -50_mm, 50_mm, 5});

    auto composite = PortalLinkBase::merge(copy(grid1), copy(grid3),
                                           BinningValue::binZ, *logger);
    BOOST_CHECK_NE(dynamic_cast<const CompositePortalLink*>(composite.get()),
                   nullptr);
  }

  BOOST_TEST_CONTEXT("Check wraparound for full circle in phi") {
    auto cyl1 = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                     30_mm, 100_mm, 180_degree);

    // z good, rphi good
    auto grid1 = GridPortalLink::make(
        cyl1, Axis{AxisClosed, -180_degree * 30_mm, 180_degree * 30_mm, 5},
        Axis{AxisBound, -100_mm, 100_mm, 5});

    auto trf2 = Transform3{Translation3{Vector3::UnitZ() * 150_mm}};
    auto cyl2 =
        Surface::makeShared<CylinderSurface>(trf2, 30_mm, 50_mm, 180_degree);

    // Second grid portal with compatible phi binning
    auto grid2 = GridPortalLink::make(
        cyl2, Axis{AxisClosed, -180_degree * 30_mm, 180_degree * 30_mm, 5},
        Axis{AxisBound, -50_mm, 50_mm, 5});

    auto mergedPtr =
        GridPortalLink::merge(*grid1, *grid2, BinningValue::binZ, *logger);
    const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
    BOOST_REQUIRE_NE(mergedPtr, nullptr);

    const auto& axis1 = *merged->grid().axes().front();
    const auto& axis2 = *merged->grid().axes().back();

    Axis axis1Expected{AxisClosed, -180_degree * 30_mm, 180_degree * 30_mm, 5};
    BOOST_CHECK_EQUAL(axis1, axis1Expected);
    Axis axis2Expected{AxisBound,
                       {-150, -110, -70, -30, 10, 50, 70, 90, 110, 130, 150}};
    BOOST_CHECK_EQUAL(axis2, axis2Expected);
  }
}

BOOST_AUTO_TEST_CASE(RPhiDirection) {
  auto cyl1 = Surface::makeShared<CylinderSurface>(Transform3::Identity(),
                                                   30_mm, 100_mm, 45_degree);

  // z good, rphi good
  auto grid1 = GridPortalLink::make(
      cyl1, Axis{AxisBound, -45_degree * 30_mm, 45_degree * 30_mm, 5},
      Axis{AxisBound, -100_mm, 100_mm, 5});
  BOOST_REQUIRE_NE(grid1, nullptr);

  auto trf2 = Transform3{AngleAxis3{90_degree, Vector3::UnitZ()}};
  auto cyl2 =
      Surface::makeShared<CylinderSurface>(trf2, 30_mm, 100_mm, 45_degree);

  // Second grid portal with compatible phi binning
  auto grid2 = GridPortalLink::make(
      cyl2, Axis{AxisBound, -45_degree * 30_mm, 45_degree * 30_mm, 5},
      Axis{AxisBound, -100_mm, 100_mm, 5});
  BOOST_REQUIRE_NE(grid2, nullptr);

  // We're merging in z direction, so the phi binnings need to be the same

  auto mergedPtr =
      GridPortalLink::merge(*grid1, *grid2, BinningValue::binRPhi, *logger);
  const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
  BOOST_REQUIRE_NE(mergedPtr, nullptr);

  const auto& axis1 = *merged->grid().axes().front();
  const auto& axis2 = *merged->grid().axes().back();
  BOOST_CHECK_CLOSE(axis1.getMin(), -90_degree * 30_mm, 1e-8);
  BOOST_CHECK_CLOSE(axis1.getMax(), 90_degree * 30_mm, 1e-8);
  BOOST_CHECK_EQUAL(axis1.getNBins(), 10);
  BOOST_CHECK_EQUAL(axis1.getType(), AxisType::Equidistant);
  BOOST_CHECK_EQUAL(axis1.getBoundaryType(), AxisBoundaryType::Bound);
  Axis axis2Expected{AxisBound, -100_mm, 100_mm, 5};
  BOOST_CHECK_EQUAL(axis2, axis2Expected);
}

BOOST_AUTO_TEST_SUITE_END()  // Merging2dCylinder

BOOST_AUTO_TEST_SUITE(Merging1dDisc)

BOOST_AUTO_TEST_SUITE(RDirection)

BOOST_AUTO_TEST_CASE(ColinearMerge) {
  // Without phi sector
  auto disc1 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm, 100_mm);

  auto grid1 = GridPortalLink::make(disc1, BinningValue::binR,
                                    Axis{AxisBound, 30_mm, 100_mm, 7});

  auto disc2 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 100_mm, 150_mm);

  auto grid2 = GridPortalLink::make(disc2, BinningValue::binR,
                                    Axis{AxisBound, 100_mm, 150_mm, 5});

  auto mergedPtr =
      GridPortalLink::merge(*grid1, *grid2, BinningValue::binR, *logger);
  BOOST_REQUIRE(mergedPtr);
  const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
  BOOST_REQUIRE_NE(merged, nullptr);

  BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
  Axis axisExpected{AxisBound, 30_mm, 150_mm, 12};
  BOOST_CHECK_EQUAL(*merged->grid().axes().front(), axisExpected);

  // With phi sector
  auto discPhi1 = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                   30_mm, 100_mm, 30_degree);

  auto discPhiGrid1 = GridPortalLink::make(discPhi1, BinningValue::binR,
                                           Axis{AxisBound, 30_mm, 100_mm, 7});

  auto discPhi2 = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                   100_mm, 150_mm, 30_degree);

  auto discPhiGrid2 = GridPortalLink::make(discPhi2, BinningValue::binR,
                                           Axis{AxisBound, 100_mm, 150_mm, 5});

  auto mergedPhiPtr = GridPortalLink::merge(*discPhiGrid1, *discPhiGrid2,
                                            BinningValue::binR, *logger);
  BOOST_REQUIRE(mergedPhiPtr);
  const auto* mergedPhi =
      dynamic_cast<const GridPortalLink*>(mergedPhiPtr.get());
  BOOST_REQUIRE_NE(mergedPhi, nullptr);

  BOOST_CHECK_EQUAL(mergedPhi->grid().axes().size(), 1);
  BOOST_CHECK_EQUAL(*mergedPhi->grid().axes().front(), axisExpected);
}

BOOST_AUTO_TEST_CASE(ParallelMerge) {
  auto disc1 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm, 100_mm);

  auto grid1 =
      GridPortalLink::make(disc1, BinningValue::binPhi,
                           Axis{AxisClosed, -180_degree, 180_degree, 5});

  auto disc2 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 100_mm, 150_mm);

  auto grid2 =
      GridPortalLink::make(disc2, BinningValue::binPhi,
                           Axis{AxisClosed, -180_degree, 180_degree, 5});

  auto mergedPtr =
      GridPortalLink::merge(*grid1, *grid2, BinningValue::binR, *logger);
  BOOST_REQUIRE(mergedPtr);
  const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
  BOOST_REQUIRE_NE(merged, nullptr);

  BOOST_CHECK_EQUAL(merged->grid().axes().size(), 2);
  const auto& axis1 = *merged->grid().axes().front();
  const auto& axis2 = *merged->grid().axes().back();
  Axis axis1Expected{AxisBound, {30_mm, 100_mm, 150_mm}};
  BOOST_CHECK_EQUAL(axis1, axis1Expected);
  Axis axis2Expected{AxisClosed, -180_degree, 180_degree, 5};
  BOOST_CHECK_EQUAL(axis2, axis2Expected);
}

BOOST_AUTO_TEST_SUITE_END()  // RDirection

BOOST_AUTO_TEST_SUITE(PhiDirection)

BOOST_AUTO_TEST_CASE(ColinearMerge) {
  auto disc1 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm,
                                                100_mm, 30_degree);

  auto grid1 = GridPortalLink::make(disc1, BinningValue::binPhi,
                                    Axis{AxisBound, -30_degree, 30_degree, 3});

  auto disc2 = Surface::makeShared<DiscSurface>(
      Transform3{AngleAxis3{90_degree, Vector3::UnitZ()}}, 30_mm, 100_mm,
      60_degree);

  auto grid2 = GridPortalLink::make(disc2, BinningValue::binPhi,
                                    Axis{AxisBound, -60_degree, 60_degree, 6});

  auto mergedPtr =
      GridPortalLink::merge(*grid1, *grid2, BinningValue::binPhi, *logger);
  BOOST_REQUIRE(mergedPtr);
  const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
  BOOST_REQUIRE_NE(merged, nullptr);

  BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
  const auto& axis = *merged->grid().axes().front();
  BOOST_CHECK_CLOSE(axis.getMin(), -90_degree, 1e-6);
  BOOST_CHECK_CLOSE(axis.getMax(), 90_degree, 1e-6);
  BOOST_CHECK_EQUAL(axis.getNBins(), 9);

  // Check wrapping

  auto disc1Half = Surface::makeShared<DiscSurface>(
      Transform3{AngleAxis3{15_degree, Vector3::UnitZ()}}, 30_mm, 100_mm,
      90_degree);

  auto grid1Half =
      GridPortalLink::make(disc1Half, BinningValue::binPhi,
                           Axis{AxisBound, -90_degree, 90_degree, 3});

  auto disc2Half = Surface::makeShared<DiscSurface>(
      Transform3{AngleAxis3{-165_degree, Vector3::UnitZ()}}, 30_mm, 100_mm,
      90_degree);

  auto grid2Half =
      GridPortalLink::make(disc2Half, BinningValue::binPhi,
                           Axis{AxisBound, -90_degree, 90_degree, 3});

  auto mergedHalfPtr = GridPortalLink::merge(*grid1Half, *grid2Half,
                                             BinningValue::binPhi, *logger);
  BOOST_REQUIRE(mergedHalfPtr);
  const auto* mergedHalf =
      dynamic_cast<const GridPortalLink*>(mergedHalfPtr.get());
  BOOST_REQUIRE_NE(mergedHalf, nullptr);

  BOOST_CHECK_EQUAL(mergedHalf->grid().axes().size(), 1);
  Axis axisHalfExpected{AxisClosed, -180_degree, 180_degree, 6};
  BOOST_CHECK_EQUAL(axisHalfExpected, *mergedHalf->grid().axes().front());
}

BOOST_AUTO_TEST_CASE(ParallelMerge) {
  auto disc1 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm,
                                                100_mm, 30_degree);

  auto grid1 = GridPortalLink::make(disc1, BinningValue::binR,
                                    Axis{AxisBound, 30_mm, 100_mm, 3});

  auto disc2 = Surface::makeShared<DiscSurface>(
      Transform3{AngleAxis3{90_degree, Vector3::UnitZ()}}, 30_mm, 100_mm,
      60_degree);

  auto grid2 = GridPortalLink::make(disc2, BinningValue::binR,
                                    Axis{AxisBound, 30_mm, 100_mm, 3});

  auto mergedPtr =
      GridPortalLink::merge(*grid1, *grid2, BinningValue::binPhi, *logger);
  BOOST_REQUIRE(mergedPtr);
  const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
  BOOST_REQUIRE_NE(merged, nullptr);

  BOOST_CHECK_EQUAL(merged->grid().axes().size(), 2);
  const auto& axis1 = *merged->grid().axes().front();
  const auto& axis2 = *merged->grid().axes().back();

  Axis axis1Expected{AxisBound, 30_mm, 100_mm, 3};
  BOOST_CHECK_EQUAL(axis1, axis1Expected);

  BOOST_CHECK_CLOSE(axis2.getMin(), -90_degree, 1e-6);
  BOOST_CHECK_CLOSE(axis2.getMax(), 90_degree, 1e-6);
  BOOST_CHECK_EQUAL(axis2.getNBins(), 2);
  BOOST_CHECK_CLOSE(axis2.getBinEdges().at(1), 30_degree, 1e-6);
  BOOST_CHECK_EQUAL(axis2.getType(), AxisType::Variable);
  BOOST_CHECK_EQUAL(axis2.getBoundaryType(), AxisBoundaryType::Bound);
}

BOOST_AUTO_TEST_SUITE_END()  // PhiDirection

BOOST_AUTO_TEST_CASE(BinFilling) {
  // Volumes for bin content checking

  auto vol1 = makeDummyVolume();
  auto vol2 = makeDummyVolume();

  BOOST_TEST_CONTEXT("RDirection") {
    auto disc1 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm,
                                                  60_mm, 30_degree);

    auto grid1 = GridPortalLink::make(disc1, BinningValue::binR,
                                      Axis{AxisBound, 30_mm, 60_mm, 2});

    grid1->setVolume(vol1.get());

    auto disc2 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 60_mm,
                                                  90_mm, 30_degree);

    auto grid2 = GridPortalLink::make(disc2, BinningValue::binR,
                                      Axis{AxisBound, 60_mm, 90_mm, 2});

    grid2->setVolume(vol2.get());

    auto mergedPtr =
        GridPortalLink::merge(*grid1, *grid2, BinningValue::binR, *logger);

    using merged_type =
        GridPortalLinkT<Axis<AxisType::Equidistant, AxisBoundaryType::Bound>>;

    const auto* merged = dynamic_cast<const merged_type*>(mergedPtr.get());
    BOOST_REQUIRE(merged);

    grid1->printContents(std::cout);
    grid2->printContents(std::cout);
    merged->printContents(std::cout);

    BOOST_CHECK_EQUAL(merged->grid().atLocalBins({1}), vol1.get());
    BOOST_CHECK_EQUAL(merged->grid().atLocalBins({2}), vol1.get());
    BOOST_CHECK_EQUAL(merged->grid().atLocalBins({3}), vol2.get());
    BOOST_CHECK_EQUAL(merged->grid().atLocalBins({4}), vol2.get());
  }

  BOOST_TEST_CONTEXT("PhiDirection") {
    auto disc1 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm,
                                                  100_mm, 30_degree);

    auto grid1 = GridPortalLink::make(
        disc1, BinningValue::binPhi, Axis{AxisBound, -30_degree, 30_degree, 2});

    grid1->setVolume(vol1.get());

    auto disc2 = Surface::makeShared<DiscSurface>(
        Transform3{AngleAxis3{60_degree, Vector3::UnitZ()}}, 30_mm, 100_mm,
        30_degree);

    auto grid2 = GridPortalLink::make(
        disc2, BinningValue::binPhi, Axis{AxisBound, -30_degree, 30_degree, 2});

    grid2->setVolume(vol2.get());

    auto mergedPtr =
        GridPortalLink::merge(*grid1, *grid2, BinningValue::binPhi, *logger);
    BOOST_REQUIRE(mergedPtr);

    using merged_type =
        GridPortalLinkT<Axis<AxisType::Equidistant, AxisBoundaryType::Bound>>;

    const auto* merged = dynamic_cast<const merged_type*>(mergedPtr.get());
    BOOST_REQUIRE(merged);

    grid1->printContents(std::cout);
    grid2->printContents(std::cout);
    merged->printContents(std::cout);

    BOOST_CHECK_EQUAL(merged->grid().atLocalBins({1}), vol2.get());
    BOOST_CHECK_EQUAL(merged->grid().atLocalBins({2}), vol2.get());
    BOOST_CHECK_EQUAL(merged->grid().atLocalBins({3}), vol1.get());
    BOOST_CHECK_EQUAL(merged->grid().atLocalBins({4}), vol1.get());
  }
}

BOOST_AUTO_TEST_SUITE_END()  // Merging1dDisc

BOOST_AUTO_TEST_SUITE(Merging2dDisc)

BOOST_AUTO_TEST_CASE(RDirection) {
  // Basic, because the parallel 1D case already tests this to some degree
  auto discPhi1 = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                   30_mm, 100_mm, 30_degree);

  auto discPhiGrid1 =
      GridPortalLink::make(discPhi1, Axis{AxisBound, 30_mm, 100_mm, 7},
                           Axis{AxisBound, -30_degree, 30_degree, 3});

  auto discPhi2 = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                   100_mm, 150_mm, 30_degree);

  auto discPhiGrid2 =
      GridPortalLink::make(discPhi2, Axis{AxisBound, 100_mm, 150_mm, 5},
                           Axis{AxisBound, -30_degree, 30_degree, 3});

  auto mergedPtr = GridPortalLink::merge(*discPhiGrid1, *discPhiGrid2,
                                         BinningValue::binR, *logger);
  BOOST_REQUIRE(mergedPtr);
  const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
  BOOST_REQUIRE_NE(merged, nullptr);
  BOOST_CHECK_EQUAL(merged->grid().axes().size(), 2);
  const auto& axis1 = *merged->grid().axes().front();
  const auto& axis2 = *merged->grid().axes().back();

  BOOST_CHECK_EQUAL(axis1.getMin(), 30_mm);
  BOOST_CHECK_EQUAL(axis1.getMax(), 150_mm);
  BOOST_CHECK_EQUAL(axis1.getNBins(), 12);
  BOOST_CHECK_EQUAL(axis1.getType(), AxisType::Equidistant);
  BOOST_CHECK_EQUAL(axis1.getBoundaryType(), AxisBoundaryType::Bound);
  BOOST_CHECK_EQUAL(axis2.getMin(), -30_degree);
  BOOST_CHECK_EQUAL(axis2.getMax(), 30_degree);
  BOOST_CHECK_EQUAL(axis2.getNBins(), 3);
  BOOST_CHECK_EQUAL(axis2.getType(), AxisType::Equidistant);
}

BOOST_AUTO_TEST_CASE(PhiDirection) {
  // Basic, because the parallel 1D case already tests this to some degree
  auto disc1 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm,
                                                100_mm, 30_degree);

  auto grid1 = GridPortalLink::make(disc1, Axis{AxisBound, 30_mm, 100_mm, 3},
                                    Axis{AxisBound, -30_degree, 30_degree, 3});

  auto disc2 = Surface::makeShared<DiscSurface>(
      Transform3{AngleAxis3{90_degree, Vector3::UnitZ()}}, 30_mm, 100_mm,
      60_degree);

  auto grid2 = GridPortalLink::make(disc2, Axis{AxisBound, 30_mm, 100_mm, 3},
                                    Axis{AxisBound, -60_degree, 60_degree, 6});

  auto mergedPtr =
      GridPortalLink::merge(*grid1, *grid2, BinningValue::binPhi, *logger);
  BOOST_REQUIRE(mergedPtr);
  const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
  BOOST_REQUIRE_NE(merged, nullptr);

  BOOST_CHECK_EQUAL(merged->grid().axes().size(), 2);
  const auto& axis1 = *merged->grid().axes().front();
  const auto& axis2 = *merged->grid().axes().back();

  BOOST_CHECK_EQUAL(axis1.getMin(), 30_mm);
  BOOST_CHECK_EQUAL(axis1.getMax(), 100_mm);
  BOOST_CHECK_EQUAL(axis1.getNBins(), 3);
  BOOST_CHECK_EQUAL(axis1.getType(), AxisType::Equidistant);
  BOOST_CHECK_EQUAL(axis1.getBoundaryType(), AxisBoundaryType::Bound);
  BOOST_CHECK_CLOSE(axis2.getMin(), -90_degree, 1e-6);
  BOOST_CHECK_CLOSE(axis2.getMax(), 90_degree, 1e-6);
  BOOST_CHECK_EQUAL(axis2.getNBins(), 9);
  BOOST_CHECK_EQUAL(axis2.getType(), AxisType::Equidistant);
  BOOST_CHECK_EQUAL(axis2.getBoundaryType(), AxisBoundaryType::Bound);
}

BOOST_AUTO_TEST_CASE(BinFilling) {
  // Volumes for bin content checking
  // Volume shape/transform is irrelevant, only used for pointer identity
  auto vol1 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto vol2 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto fillCheckerBoard = [&](auto& grid) {
    auto loc = grid.numLocalBins();
    for (std::size_t i = 1; i <= loc[0]; ++i) {
      for (std::size_t j = 1; j <= loc[1]; ++j) {
        grid.atLocalBins({i, j}) = (i + j) % 2 == 0 ? vol1.get() : vol2.get();
      }
    }
  };

  auto checkCheckerBoard = [&](const auto& grid) {
    auto loc = grid.numLocalBins();
    for (std::size_t i = 1; i <= loc[0]; ++i) {
      for (std::size_t j = 1; j <= loc[1]; ++j) {
        const auto* vol = grid.atLocalBins({i, j});
        if (vol != ((i + j) % 2 == 0 ? vol1.get() : vol2.get())) {
          BOOST_ERROR("Is not a checkerboard pattern");
          return;
        }
      }
    }
  };

  BOOST_TEST_CONTEXT("RDirection") {
    auto discPhi1 = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                     30_mm, 60_mm, 30_degree);

    auto discPhiGrid1 =
        GridPortalLink::make(discPhi1, Axis{AxisBound, 30_mm, 60_mm, 2},
                             Axis{AxisBound, -30_degree, 30_degree, 2});

    fillCheckerBoard(discPhiGrid1->grid());
    checkCheckerBoard(discPhiGrid1->grid());

    auto discPhi2 = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                     60_mm, 90_mm, 30_degree);

    auto discPhiGrid2 =
        GridPortalLink::make(discPhi2, Axis{AxisBound, 60_mm, 90_mm, 2},
                             Axis{AxisBound, -30_degree, 30_degree, 2});

    fillCheckerBoard(discPhiGrid2->grid());
    checkCheckerBoard(discPhiGrid2->grid());

    auto mergedPtr = GridPortalLink::merge(*discPhiGrid1, *discPhiGrid2,
                                           BinningValue::binR, *logger);

    using merged_type =
        GridPortalLinkT<Axis<AxisType::Equidistant, AxisBoundaryType::Bound>,
                        Axis<AxisType::Equidistant, AxisBoundaryType::Bound>>;

    const auto* merged = dynamic_cast<const merged_type*>(mergedPtr.get());
    BOOST_REQUIRE(merged);
    checkCheckerBoard(merged->grid());

    // Fill a / b
    discPhiGrid1->setVolume(vol1.get());
    discPhiGrid2->setVolume(vol2.get());

    mergedPtr = GridPortalLink::merge(*discPhiGrid1, *discPhiGrid2,
                                      BinningValue::binR, *logger);

    merged = dynamic_cast<const merged_type*>(mergedPtr.get());
    BOOST_REQUIRE(merged);

    const auto* v1 = vol1.get();
    const auto* v2 = vol2.get();

    std::vector<std::pair<Vector2, const TrackingVolume*>> locations = {
        {{40_mm, -20_degree}, v1}, {{40_mm, 20_degree}, v1},
        {{50_mm, -20_degree}, v1}, {{50_mm, 20_degree}, v1},
        {{70_mm, -20_degree}, v2}, {{70_mm, 20_degree}, v2},
        {{80_mm, -20_degree}, v2}, {{80_mm, 20_degree}, v2},
    };

    for (const auto& [loc, vol] : locations) {
      BOOST_TEST_CONTEXT(loc.transpose())
      BOOST_CHECK_EQUAL(merged->resolveVolume(gctx, loc).value(), vol);
    }

    std::vector<std::vector<const TrackingVolume*>> contents = {
        {v1, v1},
        {v1, v1},
        {v2, v2},
        {v2, v2},
    };

    for (std::size_t i = 0; i < 4; ++i) {
      for (std::size_t j = 0; j < 2; ++j) {
        BOOST_CHECK_EQUAL(merged->grid().atLocalBins({i + 1, j + 1}),
                          contents.at(i).at(j));
      }
    }
  }

  BOOST_TEST_CONTEXT("PhiDirection") {
    auto disc1 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm,
                                                  100_mm, 30_degree);

    auto grid1 =
        GridPortalLink::make(disc1, Axis{AxisBound, 30_mm, 100_mm, 2},
                             Axis{AxisBound, -30_degree, 30_degree, 2});
    fillCheckerBoard(grid1->grid());
    checkCheckerBoard(grid1->grid());

    auto disc2 = Surface::makeShared<DiscSurface>(
        Transform3{AngleAxis3{60_degree, Vector3::UnitZ()}}, 30_mm, 100_mm,
        30_degree);

    auto grid2 =
        GridPortalLink::make(disc2, Axis{AxisBound, 30_mm, 100_mm, 2},
                             Axis{AxisBound, -30_degree, 30_degree, 2});
    fillCheckerBoard(grid2->grid());
    checkCheckerBoard(grid2->grid());

    auto mergedPtr =
        GridPortalLink::merge(*grid1, *grid2, BinningValue::binPhi, *logger);

    BOOST_REQUIRE(mergedPtr);

    using merged_type =
        GridPortalLinkT<Axis<AxisType::Equidistant, AxisBoundaryType::Bound>,
                        Axis<AxisType::Equidistant, AxisBoundaryType::Bound>>;

    const auto* merged = dynamic_cast<const merged_type*>(mergedPtr.get());
    BOOST_REQUIRE(merged);

    checkCheckerBoard(merged->grid());

    // Fill a / b
    grid1->setVolume(vol1.get());
    grid2->setVolume(vol2.get());

    mergedPtr =
        GridPortalLink::merge(*grid1, *grid2, BinningValue::binPhi, *logger);
    merged = dynamic_cast<const merged_type*>(mergedPtr.get());
    BOOST_REQUIRE(merged);

    const auto* v1 = vol1.get();
    const auto* v2 = vol2.get();

    std::vector<std::pair<Vector2, const TrackingVolume*>> locations = {
        {{40_mm, -50_degree}, v2}, {{40_mm, -10_degree}, v2},
        {{50_mm, -50_degree}, v2}, {{50_mm, -10_degree}, v2},
        {{40_mm, 10_degree}, v1},  {{50_mm, 50_degree}, v1},
        {{50_mm, 10_degree}, v1},  {{50_mm, 50_degree}, v1},
    };

    for (const auto& [loc, vol] : locations) {
      BOOST_TEST_CONTEXT(loc.transpose())
      BOOST_CHECK_EQUAL(merged->resolveVolume(gctx, loc).value(), vol);
    }

    std::vector<std::vector<const TrackingVolume*>> contents = {
        {v2, v2, v1, v1},
        {v2, v2, v1, v1},
    };

    for (std::size_t i = 0; i < 2; ++i) {
      for (std::size_t j = 0; j < 4; ++j) {
        BOOST_CHECK_EQUAL(merged->grid().atLocalBins({i + 1, j + 1}),
                          contents.at(i).at(j));
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()  // Merging2dDisc

BOOST_AUTO_TEST_SUITE(MergingMixedDisc)

BOOST_AUTO_TEST_CASE(RDirection) {
  auto discPhi1 = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                   30_mm, 100_mm, 30_degree);

  auto discPhiGrid1 =
      GridPortalLink::make(discPhi1, Axis{AxisBound, 30_mm, 100_mm, 7},
                           Axis{AxisBound, -30_degree, 30_degree, 3});

  auto discPhi2 = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                   100_mm, 150_mm, 30_degree);

  auto discPhiGrid21dPhi =
      GridPortalLink::make(discPhi2, BinningValue::binPhi,
                           Axis{AxisBound, -30_degree, 30_degree, 3});

  auto merged12PhiPtr = GridPortalLink::merge(*discPhiGrid1, *discPhiGrid21dPhi,
                                              BinningValue::binR, *logger);
  BOOST_REQUIRE(merged12PhiPtr);
  const auto* merged12Phi =
      dynamic_cast<const GridPortalLink*>(merged12PhiPtr.get());
  BOOST_REQUIRE_NE(merged12Phi, nullptr);

  auto merged21PhiPtr = GridPortalLink::merge(*discPhiGrid21dPhi, *discPhiGrid1,
                                              BinningValue::binR, *logger);
  BOOST_REQUIRE(merged21PhiPtr);
  const auto* merged21Phi =
      dynamic_cast<const GridPortalLink*>(merged21PhiPtr.get());
  BOOST_REQUIRE_NE(merged21Phi, nullptr);

  BOOST_CHECK_EQUAL(merged12Phi->grid(), merged21Phi->grid());

  BOOST_CHECK_EQUAL(merged12Phi->grid().axes().size(), 2);
  const auto& axis1 = *merged12Phi->grid().axes().front();
  const auto& axis2 = *merged12Phi->grid().axes().back();

  BOOST_CHECK_EQUAL(axis1.getMin(), 30_mm);
  BOOST_CHECK_EQUAL(axis1.getMax(), 150_mm);
  BOOST_CHECK_EQUAL(axis1.getNBins(), 8);
  BOOST_CHECK_EQUAL(axis1.getType(), AxisType::Variable);
  BOOST_CHECK_EQUAL(axis1.getBoundaryType(), AxisBoundaryType::Bound);
  BOOST_CHECK_EQUAL(axis2.getMin(), -30_degree);
  BOOST_CHECK_EQUAL(axis2.getMax(), 30_degree);
  BOOST_CHECK_EQUAL(axis2.getNBins(), 3);
  BOOST_CHECK_EQUAL(axis2.getType(), AxisType::Equidistant);
  BOOST_CHECK_EQUAL(axis2.getBoundaryType(), AxisBoundaryType::Bound);

  auto discPhiGrid21dR = GridPortalLink::make(
      discPhi2, BinningValue::binR, Axis{AxisBound, 100_mm, 150_mm, 5});

  auto merged12RPtr = GridPortalLink::merge(*discPhiGrid1, *discPhiGrid21dR,
                                            BinningValue::binR, *logger);
  BOOST_REQUIRE(merged12RPtr);
  const auto* merged12R =
      dynamic_cast<const GridPortalLink*>(merged12RPtr.get());
  BOOST_REQUIRE_NE(merged12R, nullptr);

  auto merged21RPtr = GridPortalLink::merge(*discPhiGrid21dR, *discPhiGrid1,
                                            BinningValue::binR, *logger);
  BOOST_REQUIRE(merged21RPtr);
  const auto* merged21R =
      dynamic_cast<const GridPortalLink*>(merged21RPtr.get());
  BOOST_REQUIRE_NE(merged21R, nullptr);
  BOOST_CHECK_EQUAL(merged12R->grid(), merged21R->grid());

  BOOST_CHECK_EQUAL(merged12R->grid().axes().size(), 2);
  const auto& axis1R = *merged12R->grid().axes().front();
  const auto& axis2R = *merged12R->grid().axes().back();

  BOOST_CHECK_EQUAL(axis1R.getMin(), 30_mm);
  BOOST_CHECK_EQUAL(axis1R.getMax(), 150_mm);
  BOOST_CHECK_EQUAL(axis1R.getNBins(), 12);
  BOOST_CHECK_EQUAL(axis1R.getType(), AxisType::Equidistant);
  BOOST_CHECK_EQUAL(axis1R.getBoundaryType(), AxisBoundaryType::Bound);
  BOOST_CHECK_EQUAL(axis2R.getMin(), -30_degree);
  BOOST_CHECK_EQUAL(axis2R.getMax(), 30_degree);
  BOOST_CHECK_EQUAL(axis2R.getNBins(), 3);
  BOOST_CHECK_EQUAL(axis2R.getType(), AxisType::Equidistant);
  BOOST_CHECK_EQUAL(axis2R.getBoundaryType(), AxisBoundaryType::Bound);
}

BOOST_AUTO_TEST_CASE(PhiDirection) {
  auto disc1 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm,
                                                100_mm, 30_degree);

  auto grid1 = GridPortalLink::make(disc1, Axis{AxisBound, 30_mm, 100_mm, 3},
                                    Axis{AxisBound, -30_degree, 30_degree, 3});

  auto disc2 = Surface::makeShared<DiscSurface>(
      Transform3{AngleAxis3{90_degree, Vector3::UnitZ()}}, 30_mm, 100_mm,
      60_degree);

  auto grid21dPhi =
      GridPortalLink::make(disc2, BinningValue::binPhi,

                           Axis{AxisBound, -60_degree, 60_degree, 6});

  auto merged12PhiPtr =
      GridPortalLink::merge(*grid1, *grid21dPhi, BinningValue::binPhi, *logger);
  BOOST_REQUIRE(merged12PhiPtr);
  const auto* merged12Phi =
      dynamic_cast<const GridPortalLink*>(merged12PhiPtr.get());
  BOOST_REQUIRE_NE(merged12Phi, nullptr);

  BOOST_CHECK_EQUAL(merged12Phi->grid().axes().size(), 2);
  const auto& axis1 = *merged12Phi->grid().axes().front();
  const auto& axis2 = *merged12Phi->grid().axes().back();

  BOOST_CHECK_EQUAL(axis1.getMin(), 30_mm);
  BOOST_CHECK_EQUAL(axis1.getMax(), 100_mm);
  BOOST_CHECK_EQUAL(axis1.getNBins(), 3);
  BOOST_CHECK_EQUAL(axis1.getType(), AxisType::Equidistant);
  BOOST_CHECK_EQUAL(axis1.getBoundaryType(), AxisBoundaryType::Bound);
  BOOST_CHECK_CLOSE(axis2.getMin(), -90_degree, 1e-6);
  BOOST_CHECK_CLOSE(axis2.getMax(), 90_degree, 1e-6);
  BOOST_CHECK_EQUAL(axis2.getNBins(), 9);
  BOOST_CHECK_EQUAL(axis2.getType(), AxisType::Equidistant);
  BOOST_CHECK_EQUAL(axis2.getBoundaryType(), AxisBoundaryType::Bound);

  auto grid21dR = GridPortalLink::make(disc2, BinningValue::binR,
                                       Axis{AxisBound, 30_mm, 100_mm, 3});

  auto merged12RPtr =
      GridPortalLink::merge(*grid1, *grid21dR, BinningValue::binPhi, *logger);
  BOOST_REQUIRE(merged12RPtr);
  const auto* merged12R =
      dynamic_cast<const GridPortalLink*>(merged12RPtr.get());
  BOOST_REQUIRE_NE(merged12R, nullptr);

  BOOST_CHECK_EQUAL(merged12R->grid().axes().size(), 2);
  const auto& axis1R = *merged12R->grid().axes().front();
  const auto& axis2R = *merged12R->grid().axes().back();

  BOOST_CHECK_EQUAL(axis1R.getMin(), 30_mm);
  BOOST_CHECK_EQUAL(axis1R.getMax(), 100_mm);
  BOOST_CHECK_EQUAL(axis1R.getNBins(), 3);
  BOOST_CHECK_EQUAL(axis1R.getType(), AxisType::Equidistant);
  BOOST_CHECK_EQUAL(axis1R.getBoundaryType(), AxisBoundaryType::Bound);
  BOOST_CHECK_CLOSE(axis2R.getMin(), -90_degree, 1e-6);
  BOOST_CHECK_CLOSE(axis2R.getMax(), 90_degree, 1e-6);
  BOOST_CHECK_EQUAL(axis2R.getNBins(), 4);
  BOOST_CHECK_EQUAL(axis2R.getType(), AxisType::Variable);
  BOOST_CHECK_EQUAL(axis2R.getBoundaryType(), AxisBoundaryType::Bound);
}

BOOST_AUTO_TEST_SUITE_END()  // MergingMixedDisc

BOOST_AUTO_TEST_SUITE(MergingCrossDisc)

BOOST_AUTO_TEST_CASE(RDirection) {
  // Volumes for bin content checking
  auto vol1 = makeDummyVolume();
  auto vol2 = makeDummyVolume();
  auto vol3 = makeDummyVolume();
  auto vol4 = makeDummyVolume();

  auto disc1 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm,
                                                100_mm, 30_degree);

  auto grid1 = GridPortalLink::make(disc1, BinningValue::binR,
                                    Axis{AxisBound, 30_mm, 100_mm, 2});
  grid1->grid().atLocalBins({1}) = vol1.get();
  grid1->grid().atLocalBins({2}) = vol2.get();

  auto disc2 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 100_mm,
                                                150_mm, 30_degree);

  auto grid2 = GridPortalLink::make(disc2, BinningValue::binPhi,
                                    Axis{AxisBound, -30_degree, 30_degree, 2});

  grid2->grid().atLocalBins({1}) = vol3.get();
  grid2->grid().atLocalBins({2}) = vol4.get();

  auto mergedPtr =
      GridPortalLink::merge(*grid1, *grid2, BinningValue::binR, *logger);

  const auto* merged = dynamic_cast<const GridPortalLink*>(mergedPtr.get());
  BOOST_REQUIRE_NE(merged, nullptr);

  BOOST_CHECK_EQUAL(merged->grid().axes().size(), 2);
  const auto& axis1 = *merged->grid().axes().front();
  const auto& axis2 = *merged->grid().axes().back();
  Axis axisExpectedR{AxisBound, {30_mm, 65_mm, 100_mm, 150_mm}};
  Axis axisExpectedPhi{AxisBound, -30_degree, 30_degree, 2};
  BOOST_CHECK_EQUAL(axis1, axisExpectedR);
  BOOST_CHECK_EQUAL(axis2, axisExpectedPhi);

  std::vector<std::pair<Vector2, TrackingVolume*>> locations = {
      {{40_mm, -15_degree}, vol1.get()},  {{40_mm, 15_degree}, vol1.get()},
      {{90_mm, -15_degree}, vol2.get()},  {{90_mm, 15_degree}, vol2.get()},

      {{110_mm, -15_degree}, vol3.get()}, {{110_mm, 15_degree}, vol4.get()},
      {{140_mm, -15_degree}, vol3.get()}, {{140_mm, 15_degree}, vol4.get()},
  };

  for (const auto& [loc, vol] : locations) {
    BOOST_TEST_CONTEXT(loc.transpose())
    BOOST_CHECK_EQUAL(merged->resolveVolume(gctx, loc).value(), vol);
  }

  grid1->printContents(std::cout);
  grid2->printContents(std::cout);
  merged->printContents(std::cout);
}

BOOST_AUTO_TEST_CASE(PhiDirection) {
  // Volumes for bin content checking

  auto vol1 = makeDummyVolume();
  auto vol2 = makeDummyVolume();
  auto vol3 = makeDummyVolume();
  auto vol4 = makeDummyVolume();

  auto disc1 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm,
                                                100_mm, 30_degree);

  auto grid1 = GridPortalLink::make(disc1, BinningValue::binR,
                                    Axis{AxisBound, 30_mm, 100_mm, 2});

  grid1->grid().atLocalBins({1}) = vol1.get();
  grid1->grid().atLocalBins({2}) = vol2.get();

  auto disc2 = Surface::makeShared<DiscSurface>(
      Transform3{AngleAxis3{90_degree, Vector3::UnitZ()}}, 30_mm, 100_mm,
      60_degree);

  auto grid2 = GridPortalLink::make(disc2, BinningValue::binPhi,
                                    Axis{AxisBound, -60_degree, 60_degree, 2});

  grid2->grid().atLocalBins({1}) = vol3.get();
  grid2->grid().atLocalBins({2}) = vol4.get();

  auto mergedPtr =
      GridPortalLink::merge(*grid1, *grid2, BinningValue::binPhi, *logger);

  using merged_type =
      GridPortalLinkT<Axis<AxisType::Equidistant, AxisBoundaryType::Bound>,
                      Axis<AxisType::Equidistant, AxisBoundaryType::Bound>>;

  const auto* merged = dynamic_cast<const merged_type*>(mergedPtr.get());
  BOOST_REQUIRE_NE(merged, nullptr);

  BOOST_CHECK_EQUAL(merged->grid().axes().size(), 2);
  const auto& axis1 = *merged->grid().axes().front();
  const auto& axis2 = *merged->grid().axes().back();
  Axis axisExpectedR{AxisBound, 30_mm, 100_mm, 2};
  BOOST_CHECK_EQUAL(axis1, axisExpectedR);

  BOOST_CHECK_CLOSE(axis2.getMin(), -90_degree, 1e-6);
  BOOST_CHECK_CLOSE(axis2.getMax(), 90_degree, 1e-6);
  BOOST_CHECK_EQUAL(axis2.getNBins(), 3);
  BOOST_CHECK_EQUAL(axis2.getType(), AxisType::Equidistant);
  BOOST_CHECK_EQUAL(axis2.getBoundaryType(), AxisBoundaryType::Bound);

  std::vector<std::pair<Vector2, TrackingVolume*>> locations = {
      {{40_mm, 45_degree}, vol1.get()},  {{40_mm, 0_degree}, vol4.get()},
      {{40_mm, -80_degree}, vol3.get()}, {{90_mm, 45_degree}, vol2.get()},
      {{90_mm, 0_degree}, vol4.get()},   {{90_mm, -80_degree}, vol3.get()},
  };

  grid1->printContents(std::cout);
  grid2->printContents(std::cout);
  merged->printContents(std::cout);

  for (const auto& [loc, vol] : locations) {
    BOOST_TEST_CONTEXT((Vector2{loc[0], loc[1] / 1_degree}.transpose()))
    BOOST_CHECK_EQUAL(merged->resolveVolume(gctx, loc).value(), vol);
  }
}

BOOST_AUTO_TEST_SUITE_END()  // MergeCrossDisc

BOOST_AUTO_TEST_SUITE_END()  // GridMerging

BOOST_AUTO_TEST_CASE(CompositeConstruction) {
  // Arbitrary volumes for testing only
  auto vol1 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto vol2 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto vol3 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto disc1 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm, 60_mm);

  auto trivial1 = std::make_unique<TrivialPortalLink>(disc1, *vol1);

  auto disc2 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 60_mm, 90_mm);
  auto trivial2 = std::make_unique<TrivialPortalLink>(disc2, *vol2);

  auto composite = std::make_unique<CompositePortalLink>(
      copy(trivial1), copy(trivial2), BinningValue::binR);

  auto compositeCopy = std::make_unique<CompositePortalLink>(
      copy(trivial1), copy(trivial2), BinningValue::binR);

  BOOST_CHECK_EQUAL(
      composite->resolveVolume(gctx, Vector2{40_mm, 0_degree}).value(),
      vol1.get());
  BOOST_CHECK_EQUAL(
      composite->resolveVolume(gctx, Vector2{70_mm, 0_degree}).value(),
      vol2.get());

  BOOST_CHECK_EQUAL(composite->depth(), 1);

  // Test exception on different surface types
  auto cyl = Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30_mm,
                                                  40_mm);
  auto trivialCyl = std::make_unique<TrivialPortalLink>(cyl, *vol3);
  BOOST_CHECK_THROW(
      CompositePortalLink(copy(trivial1), copy(trivialCyl), BinningValue::binR),
      std::invalid_argument);

  auto disc3 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 90_mm, 120_mm);
  auto trivial3 = std::make_unique<TrivialPortalLink>(disc3, *vol3);

  // Test exception on un-mergable surfaces
  BOOST_CHECK_THROW(
      CompositePortalLink(copy(trivial1), copy(trivial3), BinningValue::binR),
      SurfaceMergingException);

  // Composite with a composite (this should work regardless of flattening)

  CompositePortalLink composite2(std::move(composite), copy(trivial3),
                                 BinningValue::binR, false);

  BOOST_CHECK_EQUAL(
      composite2.resolveVolume(gctx, Vector2{40_mm, 0_degree}).value(),
      vol1.get());
  BOOST_CHECK_EQUAL(
      composite2.resolveVolume(gctx, Vector2{70_mm, 0_degree}).value(),
      vol2.get());
  BOOST_CHECK_EQUAL(
      composite2.resolveVolume(gctx, Vector2{100_mm, 0_degree}).value(),
      vol3.get());

  // Two without flattening
  BOOST_CHECK_EQUAL(composite2.depth(), 2);

  CompositePortalLink composite2Flat(std::move(compositeCopy), copy(trivial3),
                                     BinningValue::binR, true);

  // One because of flattening
  BOOST_CHECK_EQUAL(composite2Flat.depth(), 1);

  BOOST_CHECK_EQUAL(
      composite2Flat.resolveVolume(gctx, Vector2{40_mm, 0_degree}).value(),
      vol1.get());
  BOOST_CHECK_EQUAL(
      composite2Flat.resolveVolume(gctx, Vector2{70_mm, 0_degree}).value(),
      vol2.get());
  BOOST_CHECK_EQUAL(
      composite2Flat.resolveVolume(gctx, Vector2{100_mm, 0_degree}).value(),
      vol3.get());
}

BOOST_AUTO_TEST_SUITE(PortalMerging)

BOOST_AUTO_TEST_CASE(TrivialTrivial) {}

BOOST_AUTO_TEST_CASE(TrivialGridR) {
  auto vol1 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto vol2 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto disc1 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm, 60_mm);

  auto disc2 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 60_mm, 90_mm);

  auto trivial = std::make_unique<TrivialPortalLink>(disc2, *vol2);
  BOOST_REQUIRE(trivial);

  auto gridPhi = GridPortalLink::make(disc1, BinningValue::binPhi,
                                      Axis{AxisClosed, -M_PI, M_PI, 2});
  gridPhi->setVolume(vol1.get());

  auto gridR = GridPortalLink::make(disc1, BinningValue::binR,
                                    Axis{AxisBound, 30_mm, 60_mm, 2});
  gridR->setVolume(vol1.get());

  BOOST_TEST_CONTEXT("Colinear") {
    auto merged = PortalLinkBase::merge(copy(trivial), copy(gridR),
                                        BinningValue::binR, *logger);
    BOOST_REQUIRE(merged);

    auto* mergedGrid = dynamic_cast<const GridPortalLink*>(merged.get());
    BOOST_REQUIRE(mergedGrid);

    mergedGrid->printContents(std::cout);

    BOOST_CHECK_EQUAL(mergedGrid->grid().axes().size(), 1);
    Axis axisExpected{AxisBound, {30_mm, 45_mm, 60_mm, 90_mm}};
    BOOST_CHECK_EQUAL(*mergedGrid->grid().axes().front(), axisExpected);
  }

  BOOST_TEST_CONTEXT("Orthogonal") {
    auto merged = PortalLinkBase::merge(copy(gridPhi), copy(trivial),
                                        BinningValue::binR, *logger);
    BOOST_REQUIRE(merged);

    auto* mergedGrid = dynamic_cast<const GridPortalLink*>(merged.get());
    BOOST_REQUIRE(mergedGrid);

    mergedGrid->printContents(std::cout);

    BOOST_CHECK_EQUAL(mergedGrid->grid().axes().size(), 2);
    Axis axis1Expected{AxisBound, 30_mm, 90_mm, 2};
    Axis axis2Expected{AxisClosed, -M_PI, M_PI, 2};
    BOOST_CHECK_EQUAL(*mergedGrid->grid().axes().front(), axis1Expected);
    BOOST_CHECK_EQUAL(*mergedGrid->grid().axes().back(), axis2Expected);
  }
}

BOOST_AUTO_TEST_CASE(TrivialGridPhi) {
  auto vol1 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto vol2 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto disc1 = Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm,
                                                100_mm, 30_degree);

  auto disc2 = Surface::makeShared<DiscSurface>(
      Transform3{AngleAxis3{90_degree, Vector3::UnitZ()}}, 30_mm, 100_mm,
      60_degree);

  auto trivial = std::make_unique<TrivialPortalLink>(disc2, *vol2);
  BOOST_REQUIRE(trivial);

  auto gridPhi = GridPortalLink::make(
      disc1, BinningValue::binPhi, Axis{AxisBound, -30_degree, 30_degree, 2});
  gridPhi->setVolume(vol1.get());

  auto gridR = GridPortalLink::make(disc1, BinningValue::binR,
                                    Axis{AxisBound, 30_mm, 100_mm, 2});
  gridR->setVolume(vol1.get());

  BOOST_TEST_CONTEXT("Colinear") {
    auto merged = PortalLinkBase::merge(copy(trivial), copy(gridPhi),
                                        BinningValue::binPhi, *logger);
    BOOST_REQUIRE(merged);

    auto* mergedGrid = dynamic_cast<const GridPortalLink*>(merged.get());
    BOOST_REQUIRE(mergedGrid);

    mergedGrid->printContents(std::cout);

    BOOST_CHECK_EQUAL(mergedGrid->grid().axes().size(), 1);
    const auto& axis = *mergedGrid->grid().axes().front();
    BOOST_CHECK_EQUAL(axis.getType(), AxisType::Variable);
    BOOST_CHECK_EQUAL(axis.getBoundaryType(), AxisBoundaryType::Bound);
    std::vector<ActsScalar> expectedBins{-90_degree, 30_degree, 60_degree,
                                         90_degree};
    CHECK_CLOSE_REL(axis.getBinEdges(), expectedBins, 1e-6);
  }

  BOOST_TEST_CONTEXT("Orthogonal") {
    auto merged = PortalLinkBase::merge(copy(gridR), copy(trivial),
                                        BinningValue::binPhi, *logger);
    BOOST_REQUIRE(merged);

    auto* mergedGrid = dynamic_cast<const GridPortalLink*>(merged.get());
    BOOST_REQUIRE(mergedGrid);

    mergedGrid->printContents(std::cout);

    BOOST_CHECK_EQUAL(mergedGrid->grid().axes().size(), 2);
    const auto& axis1 = *mergedGrid->grid().axes().front();
    const auto& axis2 = *mergedGrid->grid().axes().back();
    Axis axis1Expected{AxisBound, 30_mm, 100_mm, 2};
    BOOST_CHECK_EQUAL(axis1, axis1Expected);
    std::vector<ActsScalar> expectedBins{-90_degree, 30_degree, 90_degree};
    CHECK_CLOSE_REL(axis2.getBinEdges(), expectedBins, 1e-6);
  }
}

BOOST_AUTO_TEST_CASE(CompositeOther) {
  auto vol1 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto vol2 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto vol3 = std::make_shared<TrackingVolume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(30_mm, 40_mm, 100_mm));

  auto disc1 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 30_mm, 60_mm);
  auto disc2 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 60_mm, 90_mm);
  auto disc3 =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 90_mm, 120_mm);

  auto grid1 = GridPortalLink::make(disc1, *vol1, BinningValue::binR);
  auto trivial2 = std::make_unique<TrivialPortalLink>(disc2, *vol2);

  auto composite12 = std::make_unique<CompositePortalLink>(
      std::move(grid1), std::move(trivial2), BinningValue::binR);

  BOOST_CHECK_EQUAL(composite12->depth(), 1);
  BOOST_CHECK_EQUAL(composite12->size(), 2);

  auto trivial3 = std::make_unique<TrivialPortalLink>(disc3, *vol3);

  auto composite123Ptr = PortalLinkBase::merge(
      std::move(composite12), std::move(trivial3), BinningValue::binR, *logger);

  const auto* composite123 =
      dynamic_cast<const CompositePortalLink*>(composite123Ptr.get());
  BOOST_REQUIRE(composite123);

  BOOST_CHECK_EQUAL(composite123->depth(), 1);

  BOOST_CHECK_EQUAL(
      composite123->resolveVolume(gctx, Vector2{40_mm, 0_degree}).value(),
      vol1.get());
  BOOST_CHECK_EQUAL(
      composite123->resolveVolume(gctx, Vector2{70_mm, 0_degree}).value(),
      vol2.get());
  BOOST_CHECK_EQUAL(
      composite123->resolveVolume(gctx, Vector2{100_mm, 0_degree}).value(),
      vol3.get());

  BOOST_CHECK_EQUAL(composite123->size(), 3);
}

BOOST_AUTO_TEST_SUITE_END()  // PortalMerging

BOOST_AUTO_TEST_SUITE_END()  // Geometry

}  // namespace Acts::Test
