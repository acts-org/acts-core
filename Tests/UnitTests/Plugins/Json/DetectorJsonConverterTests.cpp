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
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorBuilder.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/DetectorVolumeBuilder.hpp"
#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/LayerStructureBuilder.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/VolumeStructureBuilder.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Plugins/Json/DetectorJsonConverter.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <fstream>
#include <memory>
#include <numbers>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

/// Helper method that allows to use the already existing testing
/// infrastructure with the new const-correct detector design
///
std::vector<std::shared_ptr<Acts::Surface>> unpackSurfaces(
    const std::vector<const Acts::Surface*>& surfaces) {
  std::vector<std::shared_ptr<Acts::Surface>> uSurfaces;
  uSurfaces.reserve(surfaces.size());
  for (const auto s : surfaces) {
    auto* ncs = const_cast<Acts::Surface*>(s);
    uSurfaces.push_back(ncs->getSharedPtr());
  }
  return uSurfaces;
}

Acts::DetectorJsonConverter::Options detrayOptions() {
  // Detray format test - manipulate for detray
  Acts::DetectorVolumeJsonConverter::Options detrayOptions;
  detrayOptions.transformOptions.writeIdentity = true;
  detrayOptions.transformOptions.transpose = true;
  detrayOptions.surfaceOptions.transformOptions =
      detrayOptions.transformOptions;
  detrayOptions.portalOptions.surfaceOptions = detrayOptions.surfaceOptions;
  return Acts::DetectorJsonConverter::Options{detrayOptions};
}

}  // namespace

Acts::GeometryContext tContext;
auto cGeometry = Acts::Test::CylindricalTrackingGeometry(tContext);

BOOST_AUTO_TEST_SUITE(DetectorJsonConverter)

BOOST_AUTO_TEST_CASE(SingleEmptyVolumeDetector) {
  auto portalGenerator = Acts::Experimental::defaultPortalGenerator();

  // Create a single cylindrical volume
  Acts::Transform3 nominal = Acts::Transform3::Identity();
  auto bounds = std::make_unique<Acts::CylinderVolumeBounds>(0., 50., 100.);

  auto volume = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume", nominal, std::move(bounds),
      Acts::Experimental::tryAllPortals());

  std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> volumes = {
      volume};

  Acts::Experimental::GeometryIdGenerator::Config generatorConfig;
  Acts::Experimental::GeometryIdGenerator generator(
      generatorConfig,
      Acts::getDefaultLogger("SequentialIdGenerator", Acts::Logging::VERBOSE));
  auto cache = generator.generateCache();
  for (auto& vol : volumes) {
    generator.assignGeometryId(cache, *vol);
  }

  auto detector = Acts::Experimental::Detector::makeShared(
      "Detector", volumes, Acts::Experimental::tryRootVolumes());

  auto jDetector = Acts::DetectorJsonConverter::toJson(tContext, *detector);

  std::ofstream out;
  out.open("single-empty-volume-detector.json");
  out << jDetector.dump(4);
  out.close();
}

BOOST_AUTO_TEST_CASE(SingleVolumeOneSurfaceDetector) {
  auto portalGenerator = Acts::Experimental::defaultPortalGenerator();

  // Create a single cylindrical volume
  Acts::Transform3 nominal = Acts::Transform3::Identity();
  auto bounds = std::make_unique<Acts::CylinderVolumeBounds>(0., 50., 100.);

  auto cylinderBounds = std::make_shared<Acts::CylinderBounds>(30., 90.);
  auto surface =
      Acts::Surface::makeShared<Acts::CylinderSurface>(nominal, cylinderBounds);

  auto volume = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume", nominal, std::move(bounds),
      {surface}, {}, Acts::Experimental::tryNoVolumes(),
      Acts::Experimental::tryAllPortalsAndSurfaces());

  std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> volumes = {
      volume};

  Acts::Experimental::GeometryIdGenerator::Config generatorConfig;
  Acts::Experimental::GeometryIdGenerator generator(
      generatorConfig,
      Acts::getDefaultLogger("SequentialIdGenerator", Acts::Logging::VERBOSE));
  auto cache = generator.generateCache();
  for (auto& vol : volumes) {
    generator.assignGeometryId(cache, *vol);
  }

  auto detector = Acts::Experimental::Detector::makeShared(
      "Detector", volumes, Acts::Experimental::tryRootVolumes());

  auto jDetector = Acts::DetectorJsonConverter::toJson(tContext, *detector);

  std::ofstream out;
  out.open("single-volume-one-surface-detector.json");
  out << jDetector.dump(4);
  out.close();

  out.open("single-volume-one-surface-detector-detray.json");
  out << Acts::DetectorJsonConverter::toJsonDetray(tContext, *detector,
                                                   detrayOptions())
             .dump(4);
  out.close();
}

BOOST_AUTO_TEST_CASE(BeamPipeEndcapBarrelDetector) {
  // Detector store
  Acts::Test::CylindricalTrackingGeometry::DetectorStore dStore;

  // Endcaps
  std::vector<std::shared_ptr<Acts::Experimental::IDetectorComponentBuilder>>
      endcapBuilders;
  for (auto [ie, ep] : Acts::enumerate(std::vector<double>({-710., 710.}))) {
    auto rSurfaces = cGeometry.surfacesRing(dStore, 6.4, 12.4, 36., 0.125, 0.,
                                            55., ep, 2., 22u);

    auto endcapSurfaces = std::make_shared<
        Acts::Experimental::LayerStructureBuilder::SurfacesHolder>(
        unpackSurfaces(rSurfaces));
    // Configure the layer structure builder
    Acts::Experimental::LayerStructureBuilder::Config lsConfig;
    lsConfig.auxiliary = "*** Endcap with 22 surfaces ***";
    lsConfig.surfacesProvider = endcapSurfaces;
    lsConfig.binnings = {Acts::Experimental::ProtoBinning(
        Acts::AxisDirection::AxisPhi, Acts::AxisBoundaryType::Closed,
        -std::numbers::pi, std::numbers::pi, 22u, 1u)};

    auto layerBuilder =
        std::make_shared<Acts::Experimental::LayerStructureBuilder>(
            lsConfig, Acts::getDefaultLogger("EndcapInteralsBuilder",
                                             Acts::Logging::VERBOSE));

    Acts::Experimental::VolumeStructureBuilder::Config shapeConfig;
    shapeConfig.boundValues = {18, 100, 10., std::numbers::pi, 0.};
    shapeConfig.transform =
        Acts::Transform3{Acts::Transform3::Identity()}.pretranslate(
            Acts::Vector3(0., 0., ep));
    shapeConfig.boundsType = Acts::VolumeBounds::BoundsType::eCylinder;

    auto shapeBuilder =
        std::make_shared<Acts::Experimental::VolumeStructureBuilder>(
            shapeConfig, Acts::getDefaultLogger("EndcapShapeBuilder",
                                                Acts::Logging::VERBOSE));

    Acts::Experimental::DetectorVolumeBuilder::Config dvCfg;
    dvCfg.name = "EndcapWithSurfaces_" + std::to_string(ie);
    dvCfg.externalsBuilder = shapeBuilder;
    dvCfg.internalsBuilder = layerBuilder;

    auto dvBuilder =
        std::make_shared<Acts::Experimental::DetectorVolumeBuilder>(
            dvCfg,
            Acts::getDefaultLogger("EndcapBuilder", Acts::Logging::VERBOSE));
    endcapBuilders.push_back(dvBuilder);
  }

  // Central barrel
  Acts::Experimental::VolumeStructureBuilder::Config innerShapeConfig;
  innerShapeConfig.boundValues = {18., 60., 700., std::numbers::pi, 0.};
  innerShapeConfig.boundsType = Acts::VolumeBounds::BoundsType::eCylinder;

  auto innerShapeBuilder =
      std::make_shared<Acts::Experimental::VolumeStructureBuilder>(
          innerShapeConfig,
          Acts::getDefaultLogger("InnerShapeBuilder", Acts::Logging::VERBOSE));

  Acts::Experimental::DetectorVolumeBuilder::Config ivCfg;
  ivCfg.name = "InnerBarrelGap";
  ivCfg.externalsBuilder = innerShapeBuilder;

  auto ivBuilder = std::make_shared<Acts::Experimental::DetectorVolumeBuilder>(
      ivCfg, Acts::getDefaultLogger("InnerBarrel", Acts::Logging::VERBOSE));

  ///  Barrel surfaces
  auto cSurfaces = cGeometry.surfacesCylinder(dStore, 8.4, 36., 0.15, 0.145, 72,
                                              3., 2., {32u, 14u});

  auto barrelSurfaces = std::make_shared<
      Acts::Experimental::LayerStructureBuilder::SurfacesHolder>(
      unpackSurfaces(cSurfaces));

  // Configure the layer structure builder
  Acts::Experimental::LayerStructureBuilder::Config lsConfig;
  lsConfig.auxiliary = "*** Barrel with 448 surfaces ***";
  lsConfig.surfacesProvider = barrelSurfaces;
  lsConfig.binnings = {
      Acts::Experimental::ProtoBinning{Acts::AxisDirection::AxisZ,
                                       Acts::AxisBoundaryType::Bound, -480.,
                                       480., 14u, 1u},
      Acts::Experimental::ProtoBinning(
          Acts::AxisDirection::AxisPhi, Acts::AxisBoundaryType::Closed,
          -std::numbers::pi, std::numbers::pi, 32u, 1u)};

  auto barrelBuilder =
      std::make_shared<Acts::Experimental::LayerStructureBuilder>(
          lsConfig, Acts::getDefaultLogger("BarrelInternalsBuilder",
                                           Acts::Logging::VERBOSE));

  Acts::Experimental::VolumeStructureBuilder::Config shapeConfig;
  shapeConfig.boundValues = {60., 80., 700., std::numbers::pi, 0.};
  shapeConfig.boundsType = Acts::VolumeBounds::BoundsType::eCylinder;

  auto shapeBuilder =
      std::make_shared<Acts::Experimental::VolumeStructureBuilder>(
          shapeConfig,
          Acts::getDefaultLogger("BarrelShapeBuilder", Acts::Logging::VERBOSE));

  Acts::Experimental::DetectorVolumeBuilder::Config dvCfg;
  dvCfg.name = "BarrelWithSurfaces";
  dvCfg.externalsBuilder = shapeBuilder;
  dvCfg.internalsBuilder = barrelBuilder;

  auto dvBuilder = std::make_shared<Acts::Experimental::DetectorVolumeBuilder>(
      dvCfg, Acts::getDefaultLogger("BarrelBuilder", Acts::Logging::VERBOSE));

  // Outer shape
  Acts::Experimental::VolumeStructureBuilder::Config outerShapeConfig;
  outerShapeConfig.boundValues = {80., 100., 700., std::numbers::pi, 0.};
  outerShapeConfig.boundsType = Acts::VolumeBounds::BoundsType::eCylinder;

  auto outerShapeBuilder =
      std::make_shared<Acts::Experimental::VolumeStructureBuilder>(
          outerShapeConfig,
          Acts::getDefaultLogger("OuterShapeBuilder", Acts::Logging::VERBOSE));

  Acts::Experimental::DetectorVolumeBuilder::Config ovCfg;
  ovCfg.name = "OuterBarrelGap";
  ovCfg.externalsBuilder = outerShapeBuilder;

  auto ovBuilder = std::make_shared<Acts::Experimental::DetectorVolumeBuilder>(
      ovCfg, Acts::getDefaultLogger("OuterBarrel", Acts::Logging::VERBOSE));

  // Build the combined barrel
  Acts::Experimental::CylindricalContainerBuilder::Config ccBarrelBuilderCfg;
  ccBarrelBuilderCfg.builders = {ivBuilder, dvBuilder, ovBuilder};
  ccBarrelBuilderCfg.binning = {Acts::AxisDirection::AxisR};

  auto ccBarrelBuilder =
      std::make_shared<Acts::Experimental::CylindricalContainerBuilder>(
          ccBarrelBuilderCfg,
          Acts::getDefaultLogger("BarrelBuilder", Acts::Logging::VERBOSE));

  // Builder the combined endcap barrel system
  Acts::Experimental::CylindricalContainerBuilder::Config ccBarrelEcBuilderCfg;
  ccBarrelEcBuilderCfg.builders = {endcapBuilders[0u], ccBarrelBuilder,
                                   endcapBuilders[1u]};
  ccBarrelEcBuilderCfg.binning = {Acts::AxisDirection::AxisZ};

  auto ccBarrelEndcapBuilder =
      std::make_shared<Acts::Experimental::CylindricalContainerBuilder>(
          ccBarrelEcBuilderCfg, Acts::getDefaultLogger("BarrelEndcapBuilder",
                                                       Acts::Logging::VERBOSE));

  // Beam Pipe
  Acts::Experimental::VolumeStructureBuilder::Config bpShapeConfig;
  bpShapeConfig.boundValues = {0., 18., 720., std::numbers::pi, 0.};
  bpShapeConfig.boundsType = Acts::VolumeBounds::BoundsType::eCylinder;

  auto bpShapeBuilder =
      std::make_shared<Acts::Experimental::VolumeStructureBuilder>(
          bpShapeConfig, Acts::getDefaultLogger("BeamPipeShapeBuilder",
                                                Acts::Logging::VERBOSE));

  Acts::Experimental::DetectorVolumeBuilder::Config bpCfg;
  bpCfg.name = "BeamPipe";
  bpCfg.externalsBuilder = bpShapeBuilder;

  auto bpBuilder = std::make_shared<Acts::Experimental::DetectorVolumeBuilder>(
      bpCfg, Acts::getDefaultLogger("BeamPipe", Acts::Logging::VERBOSE));

  // Full detector
  Acts::Experimental::CylindricalContainerBuilder::Config detCompBuilderCfg;
  detCompBuilderCfg.builders = {bpBuilder, ccBarrelEndcapBuilder};
  detCompBuilderCfg.binning = {Acts::AxisDirection::AxisR};

  auto detCompBuilder =
      std::make_shared<Acts::Experimental::CylindricalContainerBuilder>(
          detCompBuilderCfg);

  auto gigConfig = Acts::Experimental::GeometryIdGenerator::Config();
  auto gig =
      std::make_shared<Acts::Experimental::GeometryIdGenerator>(gigConfig);

  Acts::Experimental::DetectorBuilder::Config detBuilderCfg;
  detBuilderCfg.name = "Detector";
  detBuilderCfg.builder = detCompBuilder;
  detBuilderCfg.geoIdGenerator = gig;

  auto detBuilder =
      std::make_shared<Acts::Experimental::DetectorBuilder>(detBuilderCfg);

  auto detector = detBuilder->construct(tContext);

  auto jDetector = Acts::DetectorJsonConverter::toJson(tContext, *detector);

  std::ofstream out;

  out.open("barrel-endcap-detector.json");
  out << jDetector.dump(4);
  out.close();

  auto in = std::ifstream("barrel-endcap-detector.json",
                          std::ifstream::in | std::ifstream::binary);

  BOOST_CHECK(in.good());
  nlohmann::json jDetectorIn;
  in >> jDetectorIn;
  in.close();

  auto detectorIn =
      Acts::DetectorJsonConverter::fromJson(tContext, jDetectorIn);

  BOOST_CHECK_EQUAL(detectorIn->name(), detector->name());

  auto jDetectorInOut =
      Acts::DetectorJsonConverter::toJson(tContext, *detectorIn);

  out.open("barrel-endcap-detector-closure.json");
  out << jDetectorInOut.dump(4);
  out.close();

  auto jDetectorDetray = Acts::DetectorJsonConverter::toJsonDetray(
      tContext, *detector, detrayOptions());

  out.open("barrel-endcap-detector-detray.json");
  out << jDetectorDetray.dump(4);
  out.close();
}

BOOST_AUTO_TEST_SUITE_END()
