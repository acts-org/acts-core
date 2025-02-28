// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/detail/IndexedSurfacesGenerator.hpp"
#include "Acts/Detector/detail/ReferenceGenerators.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Navigation/NavigationStateUpdaters.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/ProtoAxis.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <numbers>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

using namespace Acts;
using namespace Acts::Test;
using namespace Acts::Experimental;
using namespace Acts::Experimental::detail;

GeometryContext tContext;
CylindricalTrackingGeometry cGeometry = CylindricalTrackingGeometry(tContext);

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(RingDisc1D) {
  // A single ring
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfaces = cGeometry.surfacesRing(dStore, 6.4, 12.4, 36., 0.125, 0.,
                                          55., 0., 2., 22u);
  // Polyhedron reference generator
  PolyhedronReferenceGenerator<1u, true> rGenerator;
  // A single proto axis clused in phi with 44 bins
  ProtoAxis pAxis(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                  -std::numbers::pi, std::numbers::pi, 44u);

  auto indexedRing =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          IndexedSurfacesNavigation, decltype(rSurfaces), decltype(rGenerator)>(
          tContext, rSurfaces, rGenerator, pAxis, 0u);

  using GridType =
      Grid<std::vector<std::size_t>,
           Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Closed>>;
  using DelegateType =
      IndexedSurfacesAllPortalsNavigation<GridType, IndexedSurfacesNavigation>;

  const auto* instance = indexedRing.instance();
  auto castedDelegate = dynamic_cast<const DelegateType*>(instance);

  BOOST_REQUIRE_NE(castedDelegate, nullptr);

  const auto& chainedUpdaters = castedDelegate->updators;
  const auto& indexedSurfaces =
      std::get<IndexedSurfacesNavigation<GridType>>(chainedUpdaters);
  const auto& grid = indexedSurfaces.grid;

  // Check that surfaces 10, 11, 12 build the bins at phi == 0
  std::vector<std::size_t> reference = {10, 11, 12};
  GridType::point_t p = {0.05};

  BOOST_CHECK(grid.atPosition(p) == reference);

  // Check that surfaces 0, 1, 21 build the bins at phi == -pi + epsilon
  reference = {0, 1, 21};
  p = {-std::numbers::pi + 0.05};
  BOOST_CHECK(grid.atPosition(p) == reference);
}

BOOST_AUTO_TEST_CASE(RingDisc1DWithSupport) {
  // A single ring
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfaces = cGeometry.surfacesRing(dStore, 6.4, 12.4, 36., 0.125, 0.,
                                          55., 0., 2., 22u);

  auto rBounds = std::make_shared<RadialBounds>(10., 20.);
  auto dSurface = Surface::makeShared<DiscSurface>(Transform3::Identity(),
                                                   std::move(rBounds));
  rSurfaces.push_back(dSurface.get());

  // Polyhedron reference generator
  PolyhedronReferenceGenerator<1u, true> rGenerator;
  // A single proto axis clused in phi with 44 bins
  ProtoAxis pAxis(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                  -std::numbers::pi, std::numbers::pi, 44u);
  auto indexedRing =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          Experimental::IndexedSurfacesNavigation>(
          tContext, rSurfaces, rGenerator, pAxis, 0u, {rSurfaces.size() - 1u});

  using GridType =
      Grid<std::vector<std::size_t>,
           Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Closed>>;

  using DelegateType =
      IndexedSurfacesAllPortalsNavigation<GridType, IndexedSurfacesNavigation>;

  const auto* instance = indexedRing.instance();
  auto castedDelegate = dynamic_cast<const DelegateType*>(instance);

  BOOST_REQUIRE_NE(castedDelegate, nullptr);

  const auto& chainedUpdaters = castedDelegate->updators;
  const auto& indexedSurfaces =
      std::get<IndexedSurfacesNavigation<GridType>>(chainedUpdaters);
  const auto& grid = indexedSurfaces.grid;

  // Check that surfaces 10, 11, 12 build the bins at phi == 0
  // Support disk now appears as 22
  std::vector<std::size_t> reference = {10, 11, 12, 22};
  GridType::point_t p = {0.05};
  BOOST_CHECK(grid.atPosition(p) == reference);

  // Check that surfaces 0, 1, 21 build the bins at phi == -pi + epsilon
  reference = {0, 1, 21, 22};
  p = {-std::numbers::pi + 0.05};
  BOOST_CHECK(grid.atPosition(p) == reference);
}

BOOST_AUTO_TEST_CASE(RingDisc2D) {
  // Two rings to make a disc
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfacesR0 = cGeometry.surfacesRing(dStore, 6.4, 12.4, 18., 0.125, 0.,
                                            42., 0., 2., 22u);

  auto rSurfacesR1 = cGeometry.surfacesRing(dStore, 12.4, 20.4, 30., 0.125, 0.,
                                            80., 0., 2., 22u);

  decltype(rSurfacesR0) rSurfaces = rSurfacesR0;
  rSurfaces.insert(rSurfaces.end(), rSurfacesR1.begin(), rSurfacesR1.end());

  ProtoAxis pAxisR(AxisDirection::AxisR, AxisBoundaryType::Bound,
                   {24., 74., 110});
  ProtoAxis pAxisPhi(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                     -std::numbers::pi, std::numbers::pi, 44u);

  PolyhedronReferenceGenerator<1u, true> rGenerator;

  auto indexedRing =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          Experimental::IndexedSurfacesNavigation>(
          tContext, rSurfaces, rGenerator, pAxisR, 0u, pAxisPhi, 0u);

  using GridType =
      Grid<std::vector<std::size_t>,
           Axis<Acts::AxisType::Variable, Acts::AxisBoundaryType::Bound>,
           Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Closed>>;

  using DelegateType =
      IndexedSurfacesAllPortalsNavigation<GridType, IndexedSurfacesNavigation>;

  const auto* instance = indexedRing.instance();
  auto castedDelegate = dynamic_cast<const DelegateType*>(instance);

  BOOST_REQUIRE_NE(castedDelegate, nullptr);

  const auto& chainedUpdaters = castedDelegate->updators;
  const auto& indexedSurfaces =
      std::get<IndexedSurfacesNavigation<GridType>>(chainedUpdaters);
  const auto& grid = indexedSurfaces.grid;

  // Check that now two rows of surfaces are given
  std::vector<std::size_t> reference = {16, 17, 38, 39};
  GridType::point_t p = {65., std::numbers::pi * 0.49};
  BOOST_CHECK(grid.atPosition(p) == reference);
}

BOOST_AUTO_TEST_CASE(RingDisc2DFine) {
  // Three rings to make a disc
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfacesR0 = cGeometry.surfacesRing(dStore, 6.4, 12.4, 18., 0.125, 0.,
                                            42., 0., 2., 22u);

  auto rSurfacesR1 = cGeometry.surfacesRing(dStore, 12.4, 20.4, 30., 0.125, 0.,
                                            80., 0., 2., 22u);

  auto rSurfacesR2 = cGeometry.surfacesRing(dStore, 18.4, 28.4, 30., 0.125, 0.,
                                            122., 0., 2., 36u);

  decltype(rSurfacesR0) rSurfaces = rSurfacesR0;
  rSurfaces.insert(rSurfaces.end(), rSurfacesR1.begin(), rSurfacesR1.end());
  rSurfaces.insert(rSurfaces.end(), rSurfacesR2.begin(), rSurfacesR2.end());

  ProtoAxis pAxisR(AxisDirection::AxisR, AxisBoundaryType::Bound, 24., 152, 8u);
  ProtoAxis pAxisPhi(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                     -std::numbers::pi, std::numbers::pi, 88u);

  PolyhedronReferenceGenerator<1u, true> rGenerator;

  auto indexedRing =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          Experimental::IndexedSurfacesNavigation>(
          tContext, rSurfaces, rGenerator, pAxisR, 0u, pAxisPhi, 0u);

  using GridType =
      Grid<std::vector<std::size_t>,
           Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Bound>,
           Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Closed>>;

  using DelegateType =
      IndexedSurfacesAllPortalsNavigation<GridType, IndexedSurfacesNavigation>;

  const auto* instance = indexedRing.instance();
  auto castedDelegate = dynamic_cast<const DelegateType*>(instance);

  BOOST_REQUIRE_NE(castedDelegate, nullptr);

  const auto& chainedUpdaters = castedDelegate->updators;
  const auto& indexedSurfaces =
      std::get<IndexedSurfacesNavigation<GridType>>(chainedUpdaters);
  const auto& grid = indexedSurfaces.grid;

  // Fine binning created fewer candidates
  std::vector<std::size_t> reference = {38, 39};
  GridType::point_t p = {80., std::numbers::pi * 0.49};
  BOOST_CHECK(grid.atPosition(p) == reference);
}

BOOST_AUTO_TEST_CASE(RingDisc2DFineExpanded) {
  // Three rings to make a disc
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfacesR0 = cGeometry.surfacesRing(dStore, 6.4, 12.4, 18., 0.125, 0.,
                                            42., 0., 2., 22u);

  auto rSurfacesR1 = cGeometry.surfacesRing(dStore, 12.4, 20.4, 30., 0.125, 0.,
                                            80., 0., 2., 22u);

  auto rSurfacesR2 = cGeometry.surfacesRing(dStore, 18.4, 28.4, 30., 0.125, 0.,
                                            122., 0., 2., 36u);

  decltype(rSurfacesR0) rSurfaces = rSurfacesR0;
  rSurfaces.insert(rSurfaces.end(), rSurfacesR1.begin(), rSurfacesR1.end());
  rSurfaces.insert(rSurfaces.end(), rSurfacesR2.begin(), rSurfacesR2.end());

  PolyhedronReferenceGenerator<1u, true> rGenerator;

  ProtoAxis pAxisR(AxisDirection::AxisR, AxisBoundaryType::Bound, 24., 152, 8u);
  ProtoAxis pAxisPhi(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                     -std::numbers::pi, std::numbers::pi, 88u);

  auto indexedRing =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          Experimental::IndexedSurfacesNavigation>(
          tContext, rSurfaces, rGenerator, pAxisR, 2u, pAxisPhi, 4u);

  using GridType =
      Grid<std::vector<std::size_t>,
           Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Bound>,
           Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Closed>>;

  using DelegateType =
      IndexedSurfacesAllPortalsNavigation<GridType, IndexedSurfacesNavigation>;

  const auto* instance = indexedRing.instance();
  auto castedDelegate = dynamic_cast<const DelegateType*>(instance);

  BOOST_REQUIRE_NE(castedDelegate, nullptr);

  const auto& chainedUpdaters = castedDelegate->updators;
  const auto& indexedSurfaces =
      std::get<IndexedSurfacesNavigation<GridType>>(chainedUpdaters);
  const auto& grid = indexedSurfaces.grid;

  // Bin expansion created again more elements
  std::vector<std::size_t> reference = {38, 39};
  GridType::point_t p = {80., std::numbers::pi * 0.49};
  BOOST_CHECK_GT(grid.atPosition(p).size(), 2u);
}

BOOST_AUTO_TEST_CASE(Cylinder2D) {
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto surfaces = cGeometry.surfacesCylinder(dStore, 8.4, 36., 0.15, 0.145,
                                             116., 3., 2., {52, 14});

  ProtoAxis pAxisZ(AxisDirection::AxisZ, AxisBoundaryType::Bound, -500., 500.,
                   28u);
  ProtoAxis pAxisPhi(AxisDirection::AxisPhi, AxisBoundaryType::Closed,
                     -std::numbers::pi, std::numbers::pi, 52u);
  PolyhedronReferenceGenerator<1u, true> rGenerator;

  auto indexedCylinder =
      Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
          Experimental::IndexedSurfacesNavigation>(
          tContext, surfaces, rGenerator, pAxisZ, 1u, pAxisPhi, 1u);

  using GridType =
      Grid<std::vector<std::size_t>,
           Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Bound>,
           Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Closed>>;

  using DelegateType =
      IndexedSurfacesAllPortalsNavigation<GridType, IndexedSurfacesNavigation>;

  const auto* instance = indexedCylinder.instance();
  auto castedDelegate = dynamic_cast<const DelegateType*>(instance);

  BOOST_REQUIRE_NE(castedDelegate, nullptr);

  const auto& chainedUpdaters = castedDelegate->updators;
  const auto& indexedSurfaces =
      std::get<IndexedSurfacesNavigation<GridType>>(chainedUpdaters);
  const auto& grid = indexedSurfaces.grid;

  // Bin expansion created again more elements
  std::vector<std::size_t> reference = {676, 677, 725, 726, 727};
  GridType::point_t p = {490., std::numbers::pi * 0.99};
  BOOST_CHECK(grid.atPosition(p) == reference);
}

BOOST_AUTO_TEST_SUITE_END()
