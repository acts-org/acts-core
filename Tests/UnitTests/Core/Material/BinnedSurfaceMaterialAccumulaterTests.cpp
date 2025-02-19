// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/BinnedSurfaceMaterialAccumulater.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <numbers>
#include <tuple>
#include <utility>
#include <vector>

namespace Acts::Test {

auto tContext = GeometryContext();

BOOST_AUTO_TEST_SUITE(BinnedSurfaceMaterialAccumulaterSuite)

BOOST_AUTO_TEST_CASE(InvalidSetupTest) {
  std::vector<std::shared_ptr<Surface>> surfaces = {
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 20.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30.0, 100.0),
  };

  for (auto [is, surface] : enumerate(surfaces)) {
    surface->assignGeometryId(GeometryIdentifier().withSensitive(is + 1));
  }

  // First is homogeneous
  MaterialSlab mp;
  surfaces[0u]->assignSurfaceMaterial(
      std::make_shared<HomogeneousSurfaceMaterial>(mp, 1.));

  // Second is empty - invalid

  BinnedSurfaceMaterialAccumulater::Config bsmaConfig;
  bsmaConfig.materialSurfaces = {surfaces[0].get(), surfaces[1].get()};
  bsmaConfig.emptyBinCorrection = true;
  bsmaConfig.geoContext = tContext;

  BinnedSurfaceMaterialAccumulater bsma(
      bsmaConfig,
      getDefaultLogger("BinnedSurfaceMaterialAccumulater", Logging::VERBOSE));

  // Generate the state - will throw and exception as the second surface is
  // invalid (w/o material)
  BOOST_CHECK_THROW(bsma.createState(), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(AccumulationTest) {
  std::vector<std::shared_ptr<Surface>> surfaces = {
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 20.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 50.0,
                                           100.0)};

  for (auto [is, surface] : enumerate(surfaces)) {
    surface->assignGeometryId(GeometryIdentifier().withSensitive(is + 1));
  }

  // Accepted materials are:
  // - homogeneous
  // - Prot0
  // - Binned (remapping)

  // First is homogeneous
  MaterialSlab mp;
  surfaces[0u]->assignSurfaceMaterial(
      std::make_shared<HomogeneousSurfaceMaterial>(mp, 1.));

  // Second surface is binned Phi / Z
  BinUtility sb1(4, -std::numbers::pi, std::numbers::pi, closed,
                 AxisDirection::AxisPhi);
  sb1 += BinUtility(2, -100., 100., open, AxisDirection::AxisZ);
  surfaces[1u]->assignSurfaceMaterial(
      std::make_shared<ProtoSurfaceMaterial>(sb1));

  // Third is binned
  std::vector<MaterialSlab> mps = {mp, mp, mp};
  BinUtility sb2(3, -100., 100., open, AxisDirection::AxisZ);
  surfaces[2u]->assignSurfaceMaterial(
      std::make_shared<BinnedSurfaceMaterial>(sb2, mps));

  BinnedSurfaceMaterialAccumulater::Config bsmaConfig;
  bsmaConfig.materialSurfaces = {surfaces[0].get(), surfaces[1].get(),
                                 surfaces[2].get()};
  bsmaConfig.emptyBinCorrection = true;
  bsmaConfig.geoContext = tContext;

  BinnedSurfaceMaterialAccumulater bsma(
      bsmaConfig,
      getDefaultLogger("BinnedSurfaceMaterialAccumulater", Logging::VERBOSE));

  // Generate the state
  auto state = bsma.createState();

  auto cState =
      static_cast<const BinnedSurfaceMaterialAccumulater::State*>(state.get());

  BOOST_CHECK_EQUAL(cState->accumulatedMaterial.size(), 3u);

  // Intersections
  // Track 0:
  // - Surface 0 hit once
  // - Surface 1 hit twice
  // - Surface 2 empty hit
  Vector3 p(0, 0, 0);
  Vector3 d0 = Vector3(50, 1, 0).normalized();

  MaterialInteraction m00;
  m00.surface = surfaces[0u].get();
  m00.position = 20 * d0;
  m00.direction = d0;

  MaterialInteraction m01;
  m01.surface = surfaces[1u].get();
  m01.position = 30 * d0;
  m01.direction = d0;

  MaterialInteraction m02;
  m02.surface = surfaces[1u].get();
  m02.position = 30 * d0;
  m02.direction = d0;

  std::vector<MaterialInteraction> mInteractions = {m01, m01, m02};
  std::vector<IAssignmentFinder::SurfaceAssignment> emptyHits = {
      {surfaces[2u].get(), 50 * d0, d0}};

  bsma.accumulate(*state, mInteractions, emptyHits);

  // Track 1:
  // - Surface 0 empty hit
  // - Surface 1 hit once
  // - Surface 2 hit once
  Vector3 d1 = Vector3(10, 10, 0).normalized();
  MaterialInteraction m11;
  m11.surface = surfaces[1u].get();
  m11.position = 30 * d1;
  m11.direction = d1;

  MaterialInteraction m12;
  m12.surface = surfaces[2u].get();
  m12.position = 50 * d1;
  m12.direction = d1;

  mInteractions = {m11, m12};
  emptyHits = {{surfaces[0u].get(), 50 * d1, d1}};
  bsma.accumulate(*state, mInteractions, emptyHits);

  // Get the maps
  auto maps = bsma.finalizeMaterial(*state);

  BOOST_CHECK_EQUAL(maps.size(), 3u);

  auto m0Itr = maps.find(GeometryIdentifier().withSensitive(1));
  BOOST_CHECK(m0Itr != maps.end());
  BOOST_CHECK(m0Itr->second != nullptr);

  auto m1Itr = maps.find(GeometryIdentifier().withSensitive(2));
  BOOST_CHECK(m1Itr != maps.end());
  BOOST_CHECK(m1Itr->second != nullptr);

  auto m2Itr = maps.find(GeometryIdentifier().withSensitive(3));
  BOOST_CHECK(m2Itr != maps.end());
  BOOST_CHECK(m2Itr->second != nullptr);

  // Check the material
  // map0 should be homogeneous
  auto m0 = m0Itr->second;
  const HomogeneousSurfaceMaterial* hm0 =
      dynamic_cast<const HomogeneousSurfaceMaterial*>(m0.get());
  BOOST_CHECK(hm0 != nullptr);

  // map1 should be binned
  auto m1 = m1Itr->second;
  const BinnedSurfaceMaterial* bm1 =
      dynamic_cast<const BinnedSurfaceMaterial*>(m1.get());
  BOOST_CHECK(bm1 != nullptr);

  // map2 should be binned
  auto m2 = m2Itr->second;
  const BinnedSurfaceMaterial* bm2 =
      dynamic_cast<const BinnedSurfaceMaterial*>(m2.get());
  BOOST_CHECK(bm2 != nullptr);

  // Check failures
  auto invalidSurface =
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 40.0, 100.0);
  invalidSurface->assignGeometryId(GeometryIdentifier().withSensitive(4));

  // Invalid surface amongst material
  MaterialInteraction mXX;
  mXX.surface = invalidSurface.get();
  mXX.position = 50 * d1;
  mXX.direction = d1;
  BOOST_CHECK_THROW(bsma.accumulate(*state, {mXX}, {}), std::invalid_argument);

  // Invalid surface amongst empty hits
  BOOST_CHECK_THROW(
      bsma.accumulate(*state, {}, {{invalidSurface.get(), 50 * d1, d1}}),
      std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
