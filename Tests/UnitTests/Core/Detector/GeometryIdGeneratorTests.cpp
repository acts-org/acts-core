// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <iostream>
#include <memory>
#include <vector>

using namespace Acts;
using namespace Acts::Experimental;

GeometryContext tContext;

namespace {

std::vector<std::shared_ptr<DetectorVolume>> createVolumes(
    std::vector<std::shared_ptr<Test::DetectorElementStub>>& detectorStore) {
  auto portalGenerator = defaultPortalGenerator();

  auto gap0VoumeBounds = std::make_unique<CylinderVolumeBounds>(0, 80, 200);

  auto gap0Volume = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Gap0Volume", Transform3::Identity(),
      std::move(gap0VoumeBounds), {}, {}, tryNoVolumes(), tryAllPortals());

  std::vector<ActsScalar> layer0Radii = {100, 102, 104, 106, 108, 110};
  auto layer0VolumeBounds =
      std::make_unique<CylinderVolumeBounds>(80, 130, 200);
  std::vector<std::shared_ptr<Surface>> layer0Surfaces = {};
  for (const auto [ir, r] : enumerate(layer0Radii)) {
    // First 4 surfaces are active
    if (ir < 4u) {
      auto detElement = std::make_shared<Test::DetectorElementStub>(
          Transform3::Identity(), std::make_shared<CylinderBounds>(r, 190),
          0.1);
      detectorStore.push_back(detElement);
      layer0Surfaces.push_back(detElement->surface().getSharedPtr());
    } else {
      // Last 2 surfaces are passive
      layer0Surfaces.push_back(Surface::makeShared<CylinderSurface>(
          Transform3::Identity(), std::make_shared<CylinderBounds>(r, 190)));
    }
  }

  auto layer0Volume = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "CylinderVolume", Transform3::Identity(),
      std::move(layer0VolumeBounds), layer0Surfaces, {}, tryNoVolumes(),
      tryAllPortalsAndSurfaces());

  auto gap1VoumeBounds = std::make_unique<CylinderVolumeBounds>(130, 200, 200);

  auto gap1Volume = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Gap1Volume", Transform3::Identity(),
      std::move(gap1VoumeBounds), {}, {}, tryNoVolumes(), tryAllPortals());

  return {gap0Volume, layer0Volume, gap1Volume};
}
}  // namespace

/// @brief  Test struct to increment the layer id by one
struct GeoIdIncrementer : public IGeometryIdGenerator {
  struct Cache {};

  /// @brief Interface method to generate a geometry id cache
  /// @return a geometry id cache wrapped in a std::any object
  IGeometryIdGenerator::GeoIdCache generateCache() const final {
    // Unfold the tuple and add the attachers
    return Cache{};
  }

  /// @brief Method for assigning a geometry id to a detector volume
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param dVolume the detector volume to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& /*cache*/,
                        DetectorVolume& dVolume) const final {
    auto vgid = dVolume.geometryId();
    vgid.setVolume(vgid.volume() + 1);
    dVolume.assignGeometryId(vgid);
  }

  /// @brief Method for assigning a geometry id to a portal
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param portal the portal to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& /*cache*/,
                        Portal& portal) const final {
    auto pgid = portal.surface().geometryId();
    pgid.setBoundary(pgid.boundary() + 1);
    portal.surface().assignGeometryId(pgid);
  }

  /// @brief Method for assigning a geometry id to a surface
  ///
  /// @param cache is the cache object for e.g. object counting
  /// @param surface the surface to assign the geometry id to
  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& /*cache*/,
                        Surface& surface) const final {
    auto sgid = surface.geometryId();
    if (sgid.sensitive() != 0u) {
      sgid.setSensitive(sgid.sensitive() + 1);
    } else {
      sgid.setPassive(sgid.passive() + 1);
    }
    surface.assignGeometryId(sgid);
  }
};

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(SequentialGeoIdGeneratorReset) {
  std::vector<std::shared_ptr<Test::DetectorElementStub>> detectorStore;

  auto volumes = createVolumes(detectorStore);

  GeometryIdGenerator::Config cfg;
  GeometryIdGenerator generator(
      cfg, getDefaultLogger("SequentialIdGenerator", Logging::VERBOSE));

  auto cache = generator.generateCache();
  for (auto& volume : volumes) {
    generator.assignGeometryId(cache, *volume);
  }

  // Checking the volume
  BOOST_CHECK_EQUAL(volumes[0]->geometryId().volume(), 1);
  for (auto [ip, p] : enumerate(volumes[0]->portals())) {
    BOOST_CHECK_EQUAL(p->surface().geometryId().boundary(), ip + 1);
  }

  BOOST_CHECK_EQUAL(volumes[1]->geometryId().volume(), 2);
  for (auto [ip, p] : enumerate(volumes[1]->portals())) {
    BOOST_CHECK_EQUAL(p->surface().geometryId().boundary(), ip + 1);
  }
  for (auto [is, s] : enumerate(volumes[1]->surfaces())) {
    if (is < 4u) {
      BOOST_CHECK_EQUAL(s->geometryId().sensitive(), is + 1);
    } else {
      BOOST_CHECK_EQUAL(s->geometryId().passive(), is - 3);
    }
  }

  BOOST_CHECK_EQUAL(volumes[2]->geometryId().volume(), 3);
  for (auto [ip, p] : enumerate(volumes[2]->portals())) {
    BOOST_CHECK_EQUAL(p->surface().geometryId().boundary(), ip + 1);
  }
}

BOOST_AUTO_TEST_CASE(SequentialGeoIdGeneratorNoReset) {
  std::vector<std::shared_ptr<Test::DetectorElementStub>> detectorStore;

  auto volumes = createVolumes(detectorStore);

  GeometryIdGenerator::Config cfg;
  cfg.resetSubCounters = false;
  GeometryIdGenerator generator(
      cfg, getDefaultLogger("SequentialIdGenerator", Logging::VERBOSE));

  auto cache = generator.generateCache();
  for (auto& volume : volumes) {
    generator.assignGeometryId(cache, *volume);
  }

  // Checking the volume
  BOOST_CHECK_EQUAL(volumes[0]->geometryId().volume(), 1);
  unsigned int portalCounter = 1;
  for (auto [ip, p] : enumerate(volumes[0]->portals())) {
    BOOST_CHECK_EQUAL(p->surface().geometryId().boundary(), portalCounter++);
  }

  BOOST_CHECK_EQUAL(volumes[1]->geometryId().volume(), 2);
  for (auto [ip, p] : enumerate(volumes[1]->portals())) {
    BOOST_CHECK_EQUAL(p->surface().geometryId().boundary(), portalCounter++);
  }

  BOOST_CHECK_EQUAL(volumes[2]->geometryId().volume(), 3);
  for (auto [ip, p] : enumerate(volumes[2]->portals())) {
    BOOST_CHECK_EQUAL(p->surface().geometryId().boundary(), portalCounter++);
  }
}

BOOST_AUTO_TEST_CASE(ContainerGeoIdGenerator) {
  std::vector<std::shared_ptr<Test::DetectorElementStub>> detectorStore;

  auto volumes = createVolumes(detectorStore);

  GeometryIdGenerator::Config cfg;
  cfg.containerMode = true;
  cfg.containerId = 15;
  GeometryIdGenerator generator(
      cfg, getDefaultLogger("ContainerIdGenerator", Logging::VERBOSE));

  auto cache = generator.generateCache();
  for (auto& volume : volumes) {
    generator.assignGeometryId(cache, *volume);
  }

  BOOST_CHECK_EQUAL(volumes[0]->geometryId().volume(), 15);
  BOOST_CHECK_EQUAL(volumes[0]->geometryId().layer(), 1);
  BOOST_CHECK_EQUAL(volumes[1]->geometryId().volume(), 15);
  BOOST_CHECK_EQUAL(volumes[1]->geometryId().layer(), 2);
  BOOST_CHECK_EQUAL(volumes[2]->geometryId().volume(), 15);
  BOOST_CHECK_EQUAL(volumes[2]->geometryId().layer(), 3);
}

BOOST_AUTO_TEST_CASE(ChainedGeoIdGenerator) {
  std::vector<std::shared_ptr<Test::DetectorElementStub>> detectorStore;

  auto volumes = createVolumes(detectorStore);

  GeometryIdGenerator::Config cfg;

  cfg.containerMode = true;
  cfg.containerId = 15;
  auto cgenerator = std::make_shared<GeometryIdGenerator>(
      cfg, getDefaultLogger("ContainerIdGenerator", Logging::VERBOSE));

  auto igenerator = std::make_shared<GeoIdIncrementer>();

  std::tuple<std::shared_ptr<const GeometryIdGenerator>,
             std::shared_ptr<const GeoIdIncrementer>>
      geoGenerators = {cgenerator, igenerator};

  ChainedGeometryIdGenerator<std::shared_ptr<const GeometryIdGenerator>,
                             std::shared_ptr<const GeoIdIncrementer>>
      generator(std::move(geoGenerators));

  auto cache = generator.generateCache();
  for (auto& volume : volumes) {
    generator.assignGeometryId(cache, *volume);
  }

  BOOST_CHECK_EQUAL(volumes[0]->geometryId().volume(), 16);
  BOOST_CHECK_EQUAL(volumes[0]->geometryId().layer(), 1);
  BOOST_CHECK_EQUAL(volumes[1]->geometryId().volume(), 16);
  BOOST_CHECK_EQUAL(volumes[1]->geometryId().layer(), 2);
  BOOST_CHECK_EQUAL(volumes[2]->geometryId().volume(), 16);
  BOOST_CHECK_EQUAL(volumes[2]->geometryId().layer(), 3);
}

BOOST_AUTO_TEST_SUITE_END()
