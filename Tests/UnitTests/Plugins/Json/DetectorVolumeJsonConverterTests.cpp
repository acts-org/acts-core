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
#include "Acts/Detector/DetectorVolumeBuilder.hpp"
#include "Acts/Detector/LayerStructureBuilder.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/VolumeStructureBuilder.hpp"
#include "Acts/Geometry/ConeVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Plugins/Json/DetectorVolumeJsonConverter.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

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

}  // namespace

Acts::GeometryContext tContext;
auto cGeometry = Acts::Test::CylindricalTrackingGeometry(tContext);

auto portalGenerator = Acts::Experimental::defaultPortalGenerator();

BOOST_AUTO_TEST_SUITE(DetectorVolumeJsonConverter)

BOOST_AUTO_TEST_CASE(SingleEmptyVolume) {
  // Create a single cylindrical volume
  Acts::Transform3 nominal = Acts::Transform3::Identity();
  auto bounds = std::make_unique<Acts::CylinderVolumeBounds>(0., 50., 100.);

  auto volume = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "EmptyVolume", nominal, std::move(bounds),
      Acts::Experimental::tryAllPortals());

  std::ofstream out;

  auto jVolume = Acts::DetectorVolumeJsonConverter::toJson(tContext, *volume,
                                                           {volume.get()});

  out.open("single-empty-volume.json");
  out << jVolume.dump(4);
  out.close();

  auto in = std::ifstream("single-empty-volume.json",
                          std::ifstream::in | std::ifstream::binary);

  BOOST_CHECK(in.good());
  nlohmann::json jVolumeIn;
  in >> jVolumeIn;
  in.close();

  auto volumeIn =
      Acts::DetectorVolumeJsonConverter::fromJson(tContext, jVolumeIn);

  BOOST_CHECK_EQUAL(volumeIn->name(), volume->name());
  BOOST_CHECK(
      volumeIn->transform(tContext).isApprox(volume->transform(tContext)));
  BOOST_CHECK_EQUAL(volumeIn->surfaces().size(), volume->surfaces().size());
  BOOST_CHECK_EQUAL(volumeIn->volumes().size(), volume->volumes().size());
}

BOOST_AUTO_TEST_CASE(SingleSurfaceVolume) {
  // Create a single cylindrical volume
  Acts::Transform3 nominal = Acts::Transform3::Identity();
  auto volumeBounds =
      std::make_unique<Acts::CylinderVolumeBounds>(0., 50., 100.);
  auto surfaceBounds = std::make_unique<Acts::CylinderBounds>(25., 100.);

  auto cylinderSurface = Acts::Surface::makeShared<Acts::CylinderSurface>(
      nominal, std::move(surfaceBounds));

  auto volume = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "CylinderVolume", nominal,
      std::move(volumeBounds), {cylinderSurface}, {},
      Acts::Experimental::tryRootVolumes(),
      Acts::Experimental::tryAllPortalsAndSurfaces());

  std::ofstream out;

  auto jVolume = Acts::DetectorVolumeJsonConverter::toJson(tContext, *volume,
                                                           {volume.get()});

  out.open("single-surface-volume.json");
  out << jVolume.dump(4);
  out.close();

  auto in = std::ifstream("single-surface-volume.json",
                          std::ifstream::in | std::ifstream::binary);

  BOOST_CHECK(in.good());
  nlohmann::json jVolumeIn;
  in >> jVolumeIn;
  in.close();

  auto volumeIn =
      Acts::DetectorVolumeJsonConverter::fromJson(tContext, jVolumeIn);

  BOOST_CHECK_EQUAL(volumeIn->name(), volume->name());
  BOOST_CHECK(
      volumeIn->transform(tContext).isApprox(volume->transform(tContext)));
  BOOST_CHECK_EQUAL(volumeIn->surfaces().size(), volume->surfaces().size());
  BOOST_CHECK_EQUAL(volumeIn->volumes().size(), volume->volumes().size());
}

BOOST_AUTO_TEST_CASE(EndcapVolumeWithSurfaces) {
  Acts::Test::CylindricalTrackingGeometry::DetectorStore dStore;

  auto rSurfaces = cGeometry.surfacesRing(dStore, 6.4, 12.4, 36., 0.125, 0.,
                                          55., -800, 2., 22u);

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
  shapeConfig.boundValues = {10, 100, 10., std::numbers::pi, 0.};
  shapeConfig.transform =
      Acts::Transform3{Acts::Transform3::Identity()}.pretranslate(
          Acts::Vector3(0., 0., -800.));
  shapeConfig.boundsType = Acts::VolumeBounds::BoundsType::eCylinder;

  auto shapeBuilder =
      std::make_shared<Acts::Experimental::VolumeStructureBuilder>(
          shapeConfig,
          Acts::getDefaultLogger("EndcapShapeBuilder", Acts::Logging::VERBOSE));

  Acts::Experimental::DetectorVolumeBuilder::Config dvCfg;
  dvCfg.auxiliary = "*** Test 1 - Cylinder with internal Surface ***";
  dvCfg.name = "CylinderWithSurface";
  dvCfg.externalsBuilder = shapeBuilder;
  dvCfg.internalsBuilder = layerBuilder;

  auto dvBuilder = std::make_shared<Acts::Experimental::DetectorVolumeBuilder>(
      dvCfg, Acts::getDefaultLogger("EndcapBuilder", Acts::Logging::VERBOSE));

  auto [volumes, portals, roots] = dvBuilder->construct(tContext);
  auto volume = volumes.front();

  auto jVolume = Acts::DetectorVolumeJsonConverter::toJson(tContext, *volume,
                                                           {volume.get()});

  std::ofstream out;
  out.open("endcap-volume-with-surfaces.json");
  out << jVolume.dump(4);
  out.close();

  auto in = std::ifstream("endcap-volume-with-surfaces.json",
                          std::ifstream::in | std::ifstream::binary);

  BOOST_CHECK(in.good());
  nlohmann::json jVolumeIn;
  in >> jVolumeIn;
  in.close();

  auto volumeIn =
      Acts::DetectorVolumeJsonConverter::fromJson(tContext, jVolumeIn);

  BOOST_CHECK_EQUAL(volumeIn->name(), volume->name());
  BOOST_CHECK(
      volumeIn->transform(tContext).isApprox(volume->transform(tContext)));
  BOOST_CHECK_EQUAL(volumeIn->surfaces().size(), volume->surfaces().size());
  BOOST_CHECK_EQUAL(volumeIn->volumes().size(), volume->volumes().size());

  // Cross-check writing
  jVolume = Acts::DetectorVolumeJsonConverter::toJson(tContext, *volumeIn,
                                                      {volumeIn.get()});
  out.open("endcap-volume-with-surfaces-closure.json");
  out << jVolume.dump(4);
  out.close();
}

BOOST_AUTO_TEST_CASE(BarrelVolumeWithSurfaces) {
  Acts::Test::CylindricalTrackingGeometry::DetectorStore dStore;
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
  shapeConfig.boundValues = {60., 80., 800., std::numbers::pi, 0.};
  shapeConfig.boundsType = Acts::VolumeBounds::BoundsType::eCylinder;

  auto shapeBuilder =
      std::make_shared<Acts::Experimental::VolumeStructureBuilder>(
          shapeConfig,
          Acts::getDefaultLogger("BarrelShapeBuilder", Acts::Logging::VERBOSE));

  Acts::Experimental::DetectorVolumeBuilder::Config dvCfg;
  dvCfg.auxiliary = "*** Test 1 - Cylinder with internal Surface ***";
  dvCfg.name = "BarrelWithSurfaces";
  dvCfg.externalsBuilder = shapeBuilder;
  dvCfg.internalsBuilder = barrelBuilder;

  auto dvBuilder = std::make_shared<Acts::Experimental::DetectorVolumeBuilder>(
      dvCfg, Acts::getDefaultLogger("EndcapBuilder", Acts::Logging::VERBOSE));

  auto [volumes, portals, roots] = dvBuilder->construct(tContext);

  auto volume = volumes.front();

  auto jVolume = Acts::DetectorVolumeJsonConverter::toJson(tContext, *volume,
                                                           {volume.get()});

  std::ofstream out;
  out.open("barrel-volume-with-surfaces.json");
  out << jVolume.dump(4);
  out.close();

  auto in = std::ifstream("barrel-volume-with-surfaces.json",
                          std::ifstream::in | std::ifstream::binary);

  BOOST_CHECK(in.good());
  nlohmann::json jVolumeIn;
  in >> jVolumeIn;
  in.close();

  auto volumeIn =
      Acts::DetectorVolumeJsonConverter::fromJson(tContext, jVolumeIn);

  BOOST_CHECK_EQUAL(volumeIn->name(), volume->name());
  BOOST_CHECK(
      volumeIn->transform(tContext).isApprox(volume->transform(tContext)));
  BOOST_CHECK_EQUAL(volumeIn->surfaces().size(), volume->surfaces().size());
  BOOST_CHECK_EQUAL(volumeIn->volumes().size(), volume->volumes().size());

  // Cross-check writing
  jVolume = Acts::DetectorVolumeJsonConverter::toJson(tContext, *volumeIn,
                                                      {volumeIn.get()});
  out.open("barrel-volume-with-surfaces-closure.json");
  out << jVolume.dump(4);
  out.close();
}

BOOST_AUTO_TEST_SUITE_END()
