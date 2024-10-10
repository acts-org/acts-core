// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Detector/ProtoDetector.hpp"
#include "Acts/Geometry/CylinderVolumeHelper.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/KDTreeTrackingGeometryBuilder.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingVolumeArrayCreator.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Geometry)

BOOST_AUTO_TEST_CASE(KDTreeTrackingGeometryBuilder_simple) {
  GeometryContext tContext;
  CylindricalTrackingGeometry ctGeometry(tContext);
  CylindricalTrackingGeometry::DetectorStore detectorStore;

  // The collected surfaces
  std::vector<std::shared_ptr<Surface>> layerSurfacePtrs;

  // Add a beam pipe
  auto hTransform = Transform3::Identity();
  layerSurfacePtrs.push_back(
      Surface::makeShared<CylinderSurface>(hTransform, 15., 800.));

  // Pixel Surfaces
  std::vector<ActsScalar> pLayerRadii = {32., 72., 116., 172.};
  std::vector<std::pair<int, int>> pLayerBinning = {
      {16, 14}, {32, 14}, {52, 14}, {78, 14}};
  std::vector<ActsScalar> pModuleTiltPhi = {0.145, 0.145, 0.145, 0.145};
  std::vector<ActsScalar> pModuleHalfX = {8.4, 8.4, 8.4, 8.4};
  std::vector<ActsScalar> pModuleHalfY = {36., 36., 36., 36.};
  std::vector<ActsScalar> pModuleThickness = {0.15, 0.15, 0.15, 0.15};

  // Fill surfaces from cylinder layers
  for (std::size_t ilp = 0; ilp < pLayerRadii.size(); ++ilp) {
    std::vector<const Surface*> layerSurfaces = ctGeometry.surfacesCylinder(
        detectorStore, pModuleHalfX[ilp], pModuleHalfY[ilp],
        pModuleThickness[ilp], pModuleTiltPhi[ilp], pLayerRadii[ilp], 2_mm,
        5_mm, pLayerBinning[ilp]);

    // Make a shared version out of it
    for (auto& sf : layerSurfaces) {
      Surface* mutableSf = const_cast<Surface*>(sf);
      layerSurfacePtrs.push_back(mutableSf->getSharedPtr());
    }
  }

  // Fill surfaces for disc layers
  std::vector<ActsScalar> discZ = {-700., -600., 600., 700.};
  std::vector<ActsScalar> discRadii = {60., 60., 60., 60.};
  std::vector<int> discModules = {22, 22, 22, 22};

  std::vector<ActsScalar> dModuleHalfXMinY = {6.4, 6.4, 6.4, 6.4};
  std::vector<ActsScalar> dModuleHalfXMaxY = {12.4, 12.4, 12.4, 12.4};
  std::vector<ActsScalar> dModuleHalfY = {36., 36., 36., 36.};
  std::vector<ActsScalar> dModuleTilt = {0.075, 0.075, 0.075, 0.075};
  std::vector<ActsScalar> dModuleThickness = {0.15, 0.15, 0.15, 0.15};

  for (std::size_t ilp = 0; ilp < discZ.size(); ++ilp) {
    std::vector<const Surface*> layerSurfaces = ctGeometry.surfacesRing(
        detectorStore, dModuleHalfXMinY[ilp], dModuleHalfXMaxY[ilp],
        dModuleHalfY[ilp], dModuleThickness[ilp], dModuleTilt[ilp],
        discRadii[ilp], discZ[ilp], 2., discModules[ilp]);
    for (auto& sf : layerSurfaces) {
      Surface* mutableSf = const_cast<Surface*>(sf);
      layerSurfacePtrs.push_back(mutableSf->getSharedPtr());
    }
  }

  // Make a proto detectpr description
  Acts::ProtoVolume beamPipeContainer;
  beamPipeContainer.name = "odd-beam-pipe";
  beamPipeContainer.extent.set(Acts::BinningValue::binR, 0., 17);
  Acts::ProtoVolume beamPipe;
  beamPipe.name = "odd-beam-pipe-l";
  beamPipe.extent.set(Acts::BinningValue::binR, 2., 16.);
  beamPipe.internal = Acts::ProtoVolume::InternalStructure{
      Acts::Surface::SurfaceType::Cylinder};
  beamPipeContainer.container = Acts::ProtoVolume::ContainerStructure{
      {beamPipe},
      {Acts::BinningData(Acts::open, Acts::BinningValue::binR, {0., 1.})},
      true};

  // Pixel section
  Acts::ProtoVolume pixelContainer;
  pixelContainer.name = "odd-pixel";
  pixelContainer.extent.set(Acts::BinningValue::binR, 18., 200);

  Acts::ProtoVolume pixelNec;
  pixelNec.name = "odd-pixel-nec";
  pixelNec.extent.set(Acts::BinningValue::binZ, -1000., -580);

  Acts::ProtoVolume pixNecD1;
  pixNecD1.name = "odd-pixel-nec-d1";
  pixNecD1.extent.set(Acts::BinningValue::binZ, -720., -680);
  Acts::ProtoVolume pixNecD0;
  pixNecD0.name = "odd-pixel-nec-d0";
  pixNecD0.extent.set(Acts::BinningValue::binZ, -620., -580);
  pixelNec.container = Acts::ProtoVolume::ContainerStructure{
      {pixNecD1, pixNecD0},
      {Acts::BinningData(Acts::open, Acts::BinningValue::binZ, {0., 1.})},
      true};
  for (auto& cv : pixelNec.container.value().constituentVolumes) {
    cv.internal =
        Acts::ProtoVolume::InternalStructure{Acts::Surface::SurfaceType::Disc};
  }

  Acts::ProtoVolume pixelBarrel;
  pixelBarrel.name = "odd-pixel-barrel";
  pixelBarrel.extent.set(Acts::BinningValue::binZ, -580., 580);

  Acts::ProtoVolume pixBarrelL0;
  pixBarrelL0.name = "odd-pixel-barrel-l0";
  pixBarrelL0.extent.set(Acts::BinningValue::binR, 28., 48.);
  pixBarrelL0.extent.set(Acts::BinningValue::binZ, -580., 580);
  pixBarrelL0.internal = Acts::ProtoVolume::InternalStructure{
      Acts::Surface::SurfaceType::Cylinder};
  Acts::ProtoVolume pixBarrelL1;
  pixBarrelL1.name = "odd-pixel-barrel-l1";
  pixBarrelL1.extent.set(Acts::BinningValue::binR, 62., 76);
  pixBarrelL1.extent.set(Acts::BinningValue::binZ, -580., 580);
  pixBarrelL1.internal = Acts::ProtoVolume::InternalStructure{
      Acts::Surface::SurfaceType::Cylinder};
  Acts::ProtoVolume pixBarrelL2;
  pixBarrelL2.name = "odd-pixel-barrel-l2";
  pixBarrelL2.extent.set(Acts::BinningValue::binR, 100., 120.);
  pixBarrelL2.extent.set(Acts::BinningValue::binZ, -580., 580);
  pixBarrelL2.internal = Acts::ProtoVolume::InternalStructure{
      Acts::Surface::SurfaceType::Cylinder};
  Acts::ProtoVolume pixBarrelL3;
  pixBarrelL3.name = "odd-pixel-barrel-l3";
  pixBarrelL3.extent.set(Acts::BinningValue::binR, 160., 180.);
  pixBarrelL3.extent.set(Acts::BinningValue::binZ, -580., 580);
  pixBarrelL3.internal = Acts::ProtoVolume::InternalStructure{
      Acts::Surface::SurfaceType::Cylinder};

  pixelBarrel.container = Acts::ProtoVolume::ContainerStructure{
      {pixBarrelL0, pixBarrelL1, pixBarrelL2, pixBarrelL3},
      {Acts::BinningData(Acts::open, Acts::BinningValue::binR, {0., 1})},
      true};

  Acts::ProtoVolume pixelPec;
  pixelPec.name = "odd-pixel-pec";
  pixelPec.extent.set(Acts::BinningValue::binZ, 580., 1000.);

  Acts::ProtoVolume pixPecD0;
  pixPecD0.name = "odd-pixel-pec-d0";
  pixPecD0.extent.set(Acts::BinningValue::binZ, 580., 620);
  Acts::ProtoVolume pixPecD1;
  pixPecD1.name = "odd-pixel-pec-d1";
  pixPecD1.extent.set(Acts::BinningValue::binZ, 680., 720);

  pixelPec.container = Acts::ProtoVolume::ContainerStructure{
      {pixPecD0, pixPecD1},
      {Acts::BinningData(Acts::open, Acts::BinningValue::binZ, {0., 1})},
      true};
  for (auto& cv : pixelPec.container.value().constituentVolumes) {
    cv.internal =
        Acts::ProtoVolume::InternalStructure{Acts::Surface::SurfaceType::Disc};
  }

  pixelContainer.container = Acts::ProtoVolume::ContainerStructure{
      {pixelNec, pixelBarrel, pixelPec},
      {Acts::BinningData(Acts::open, Acts::BinningValue::binZ,
                         {-1000., -580., 580., 1000.})}};

  Acts::ProtoVolume detectorContainer;
  detectorContainer.name = "odd-detector";
  detectorContainer.extent.set(Acts::BinningValue::binR, 0., 200);
  detectorContainer.container = Acts::ProtoVolume::ContainerStructure{
      {beamPipeContainer, pixelContainer},
      {Acts::BinningData(Acts::open, Acts::BinningValue::binR,
                         {0., 17.5, 200.})}};

  Acts::ProtoDetector detector;
  detector.name = "odd";
  detector.worldVolume = detectorContainer;

  auto logLevel = Acts::Logging::VERBOSE;

  // Surface array creator
  auto surfaceArrayCreator = std::make_shared<const Acts::SurfaceArrayCreator>(
      Acts::SurfaceArrayCreator::Config(),
      Acts::getDefaultLogger("SurfaceArrayCreator", logLevel));
  // Layer Creator
  Acts::LayerCreator::Config lcConfig;
  lcConfig.surfaceArrayCreator = surfaceArrayCreator;
  auto layerCreator = std::make_shared<Acts::LayerCreator>(
      lcConfig, Acts::getDefaultLogger("LayerCreator", logLevel));
  // Layer array creator
  Acts::LayerArrayCreator::Config lacConfig;
  auto layerArrayCreator = std::make_shared<const Acts::LayerArrayCreator>(
      lacConfig, Acts::getDefaultLogger("LayerArrayCreator", logLevel));
  // Tracking volume array creator
  Acts::TrackingVolumeArrayCreator::Config tvacConfig;
  auto tVolumeArrayCreator =
      std::make_shared<const Acts::TrackingVolumeArrayCreator>(
          tvacConfig,
          Acts::getDefaultLogger("TrackingVolumeArrayCreator", logLevel));
  // configure the cylinder volume helper
  Acts::CylinderVolumeHelper::Config cvhConfig;
  cvhConfig.layerArrayCreator = layerArrayCreator;
  cvhConfig.trackingVolumeArrayCreator = tVolumeArrayCreator;
  auto cylinderVolumeHelper =
      std::make_shared<const Acts::CylinderVolumeHelper>(
          cvhConfig, Acts::getDefaultLogger("CylinderVolumeHelper", logLevel));

  // The KDT tracking geometry builder
  Acts::KDTreeTrackingGeometryBuilder::Config kdtgConfig;
  kdtgConfig.layerCreator = layerCreator;
  kdtgConfig.trackingVolumeHelper = cylinderVolumeHelper;
  // Reserve the right amount of surfaces
  kdtgConfig.surfaces = layerSurfacePtrs;

  // Assign the proto detector
  kdtgConfig.protoDetector = detector;

  // Make the builder
  auto kdtTrackingGeometryBuilder = Acts::KDTreeTrackingGeometryBuilder(
      kdtgConfig,
      Acts::getDefaultLogger("KDTreeTrackingGeometryBuilder", logLevel));

  auto trackingGeometry = kdtTrackingGeometryBuilder.trackingGeometry(tContext);
  BOOST_CHECK(trackingGeometry != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
