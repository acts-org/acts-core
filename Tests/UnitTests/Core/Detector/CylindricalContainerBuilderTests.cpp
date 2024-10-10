// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/CylindricalContainerBuilder.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Detector/interface/IDetectorComponentBuilder.hpp"
#include "Acts/Detector/interface/IGeometryIdGenerator.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalDetector.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <any>
#include <cmath>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace Acts;
using namespace Acts::Test;
using namespace Acts::Experimental;

GeometryContext tContext;

class VolumeGeoIdGenerator : public IGeometryIdGenerator {
 public:
  struct Cache {
    unsigned int volumeCount = 0;
  };

  IGeometryIdGenerator::GeoIdCache generateCache() const final {
    return Cache{0};
  }

  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& cache,
                        DetectorVolume& dVolume) const final {
    auto& ccache = std::any_cast<Cache&>(cache);
    ccache.volumeCount += 1;
    Acts::GeometryIdentifier geoID;
    geoID.setVolume(ccache.volumeCount);
    dVolume.assignGeometryId(geoID);
  }

  void assignGeometryId(
      Acts::Experimental::IGeometryIdGenerator::GeoIdCache& /*cache*/,
      Acts::Experimental::Portal& /*portal*/) const final {}

  void assignGeometryId(
      Acts::Experimental::IGeometryIdGenerator::GeoIdCache& /*cache*/,
      Acts::Surface& /*surface*/) const final {}
};

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(CylindricaContainerBuilder_Misconfiguration) {
  // misconfiruation: no builders
  CylindricalContainerBuilder::Config misCfg;
  BOOST_CHECK_THROW(auto a = CylindricalContainerBuilder(misCfg),
                    std::invalid_argument);
  // misconfiguration - 1D binning not in z, r, phi
  misCfg.builders = {nullptr};
  misCfg.binning = {Acts::BinningValue::binX};
  BOOST_CHECK_THROW(auto b = CylindricalContainerBuilder(misCfg),
                    std::invalid_argument);

  // misconfiguration - 2D binning not in z, r,
  misCfg.builders = {nullptr, nullptr};
  misCfg.binning = {Acts::BinningValue::binZ, Acts::BinningValue::binPhi};
  BOOST_CHECK_THROW(auto c = CylindricalContainerBuilder(misCfg),
                    std::invalid_argument);

  // misconfiguration - 2D binning  in z, r, but not exactly 2 builders
  misCfg.builders = {nullptr, nullptr, nullptr};
  misCfg.binning = {Acts::BinningValue::binZ, Acts::BinningValue::binR};
  BOOST_CHECK_THROW(auto d = CylindricalContainerBuilder(misCfg),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(CylindricaContainerBuildingZ) {
  // Declare a negative disc builder
  Transform3 negZ = Transform3::Identity();
  negZ.pretranslate(Vector3(0., 0., -300.));
  auto negDisc =
      std::make_shared<CylindricalVolumeBuilder<DiscSurface, RadialBounds>>(
          negZ, CylinderVolumeBounds(50., 200., 100.), RadialBounds(60., 190.),
          "NegativeDisc");

  // Declare a barrel builder
  auto barrel = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(50., 200., 200.),
      CylinderBounds(80., 190.), "Barrel");
  // Declare a positive disc builder
  Transform3 posZ = Transform3::Identity();
  posZ.pretranslate(Vector3(0., 0., 300.));
  auto posDisc =
      std::make_shared<CylindricalVolumeBuilder<DiscSurface, RadialBounds>>(
          posZ, CylinderVolumeBounds(50., 200., 100.), RadialBounds(60., 190.),
          "PositiveDisc");

  // Create the container builder
  CylindricalContainerBuilder::Config tripleZCfg;
  tripleZCfg.auxiliary = "*** Test 0 - Build triple in Z ***";
  tripleZCfg.builders = {negDisc, barrel, posDisc};
  tripleZCfg.binning = {BinningValue::binZ};
  tripleZCfg.geoIdGenerator = std::make_shared<VolumeGeoIdGenerator>();
  // Create a materialBinning
  tripleZCfg.portalMaterialBinning[2u] = BinningDescription{
      {ProtoBinning(BinningValue::binZ, Acts::AxisBoundaryType::Bound, 50),
       ProtoBinning(BinningValue::binPhi, Acts::AxisBoundaryType::Closed, -M_PI,
                    M_PI, 12)}};

  // Let's test the reverse generation
  tripleZCfg.geoIdReverseGen = true;

  auto tripleZ = std::make_shared<CylindricalContainerBuilder>(
      tripleZCfg, getDefaultLogger("TripleBuilderZ", Logging::VERBOSE));

  auto [volumes, portals, roots] = tripleZ->construct(tContext);

  BOOST_CHECK_EQUAL(portals.size(), 4u);
  BOOST_CHECK_EQUAL(roots.volumes.size(), 3u);
  BOOST_CHECK_EQUAL(roots.volumes[0]->geometryId().volume(), 3u);
  BOOST_CHECK_EQUAL(roots.volumes[1]->geometryId().volume(), 2u);
  BOOST_CHECK_EQUAL(roots.volumes[2]->geometryId().volume(), 1u);

  // The outside surface should have a proto material description now
  BOOST_CHECK_NE(portals[2u]->surface().surfaceMaterial(), nullptr);
  // others should not have a proto material description
  BOOST_CHECK_EQUAL(portals[0u]->surface().surfaceMaterial(), nullptr);
  BOOST_CHECK_EQUAL(portals[1u]->surface().surfaceMaterial(), nullptr);
  BOOST_CHECK_EQUAL(portals[3u]->surface().surfaceMaterial(), nullptr);
}

BOOST_AUTO_TEST_CASE(CylindricaContainerBuildingR) {
  // Declare a barrel builder
  auto barrel0 = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(50., 80., 200.),
      CylinderBounds(65., 190.), "Barrel0");

  // Declare a barrel builder
  auto barrel1 = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(80., 110., 200.),
      CylinderBounds(95., 190.), "Barrel1");

  // Declare a barrel builder
  auto barrel2 = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(110., 140., 200.),
      CylinderBounds(125., 190.), "Barrel2");

  // Create the container builder
  CylindricalContainerBuilder::Config barrelRCfg;
  barrelRCfg.auxiliary = "*** Test 1 - Build multilayer barrel ***";
  barrelRCfg.builders = {barrel0, barrel1, barrel2};
  barrelRCfg.binning = {BinningValue::binR};
  barrelRCfg.geoIdGenerator = std::make_shared<VolumeGeoIdGenerator>();

  auto barrelR = std::make_shared<CylindricalContainerBuilder>(
      barrelRCfg, getDefaultLogger("BarrelBuilderR", Logging::VERBOSE));

  auto [volumes, portals, roots] = barrelR->construct(tContext);

  BOOST_CHECK_EQUAL(portals.size(), 4u);
  BOOST_CHECK_EQUAL(roots.volumes.size(), 3u);
  BOOST_CHECK_EQUAL(roots.volumes[0]->geometryId().volume(), 1u);
  BOOST_CHECK_EQUAL(roots.volumes[1]->geometryId().volume(), 2u);
  BOOST_CHECK_EQUAL(roots.volumes[2]->geometryId().volume(), 3u);
}

BOOST_AUTO_TEST_CASE(CylindricaContainerBuildingPhi) {
  // Create the container builder
  CylindricalContainerBuilder::Config barrelPhiCfg;
  barrelPhiCfg.auxiliary = "*** Test 2 - Build segmented phi barrel ***";
  barrelPhiCfg.binning = {BinningValue::binPhi};

  unsigned int phiSectors = 5;
  Acts::ActsScalar phiHalfSector = M_PI / phiSectors;

  std::vector<std::shared_ptr<DetectorVolume>> phiVolumes = {};
  for (unsigned int i = 0; i < phiSectors; ++i) {
    // The volume bounds
    Acts::CylinderVolumeBounds volumeBounds(
        10., 100., 100., phiHalfSector, -M_PI + (2u * i + 1u) * phiHalfSector);
    // The surface boudns
    Acts::CylinderBounds surfaceBounds(50., 90., 0.99 * phiHalfSector,
                                       -M_PI + (2u * i + 1u) * phiHalfSector);

    auto builder = std::make_shared<
        CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
        Transform3::Identity(), volumeBounds, surfaceBounds,
        std::string("Sector_") + std::to_string(i));
    barrelPhiCfg.builders.push_back(builder);
  }

  auto barrelPhi = std::make_shared<CylindricalContainerBuilder>(
      barrelPhiCfg, getDefaultLogger("BarrelBuilderPhi", Logging::VERBOSE));

  auto [volumes, portals, roots] = barrelPhi->construct(tContext);

  BOOST_CHECK_EQUAL(portals.size(), 4u);
  BOOST_CHECK_EQUAL(roots.volumes.size(), 5u);
}

BOOST_AUTO_TEST_CASE(CylindricalContainerBuilderDetector) {
  // Declare a barrel sub builder
  auto beampipe = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(0., 50., 600.),
      CylinderBounds(25., 590.), "BeamPipe");

  // Declare a negative disc builder
  Transform3 negZ = Transform3::Identity();
  negZ.pretranslate(Vector3(0., 0., -300.));
  auto endcapN =
      std::make_shared<CylindricalVolumeBuilder<DiscSurface, RadialBounds>>(
          negZ, CylinderVolumeBounds(50., 140., 100.), RadialBounds(60., 130.),
          "NegativeEndcap");

  // Declare a barrel sub builder
  auto barrel0 = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(50., 80., 200.),
      CylinderBounds(65., 190.), "Barrel0");

  // Declare a barrel sub builder
  auto barrel1 = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(80., 110., 200.),
      CylinderBounds(95., 190.), "Barrel1");

  // Declare a barrel sub builder
  auto barrel2 = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(110., 140., 200.),
      CylinderBounds(125., 190.), "Barrel2");

  // Create the barrel container builder
  CylindricalContainerBuilder::Config barrelRCfg;
  barrelRCfg.builders = {barrel0, barrel1, barrel2};
  barrelRCfg.binning = {BinningValue::binR};

  auto barrel = std::make_shared<CylindricalContainerBuilder>(
      barrelRCfg, getDefaultLogger("BarrelBuilderR", Logging::VERBOSE));

  Transform3 posZ = Transform3::Identity();
  posZ.pretranslate(Vector3(0., 0., 300.));
  auto endcapP =
      std::make_shared<CylindricalVolumeBuilder<DiscSurface, RadialBounds>>(
          posZ, CylinderVolumeBounds(50., 140., 100.), RadialBounds(60., 130.),
          "PositiveEndcap");

  // Create the barrel container builder
  CylindricalContainerBuilder::Config barrelEndcapCfg;
  barrelEndcapCfg.builders = {endcapN, barrel, endcapP};
  barrelEndcapCfg.binning = {BinningValue::binZ};

  auto barrelEndcap = std::make_shared<CylindricalContainerBuilder>(
      barrelEndcapCfg,
      getDefaultLogger("BarrelEndcapBuilder", Logging::VERBOSE));

  // Create the barrel container builder
  CylindricalContainerBuilder::Config detectorCfg;
  detectorCfg.builders = {beampipe, barrelEndcap};
  detectorCfg.binning = {BinningValue::binR};

  auto detector = std::make_shared<CylindricalContainerBuilder>(
      detectorCfg, getDefaultLogger("DetectorBuilder", Logging::VERBOSE));

  auto [volumes, portals, roots] = detector->construct(tContext);
  BOOST_CHECK_EQUAL(portals.size(), 3u);
  BOOST_CHECK_EQUAL(roots.volumes.size(), 6u);
}

BOOST_AUTO_TEST_SUITE_END()
