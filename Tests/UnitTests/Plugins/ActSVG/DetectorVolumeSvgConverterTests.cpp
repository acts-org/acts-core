// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/DetectorVolumeBuilder.hpp"
#include "Acts/Detector/LayerStructureBuilder.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/VolumeStructureBuilder.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/NavigationStateUpdaters.hpp"
#include "Acts/Plugins/ActSVG/DetectorVolumeSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/IndexedSurfacesSvgConverter.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <fstream>
#include <memory>
#include <numbers>
#include <vector>

namespace {
/// Helper method that allows to use the already existing testing
/// infrastructure with the new const-correct detector design
///
std::vector<std::shared_ptr<Acts::Surface>> unpackSurfaces(
    const std::vector<const Acts::Surface*>& surfaces) {
  std::vector<std::shared_ptr<Acts::Surface>> uSurfaces;
  uSurfaces.reserve(surfaces.size());
  for (const auto& s : surfaces) {
    auto* ncs = const_cast<Acts::Surface*>(s);
    uSurfaces.push_back(ncs->getSharedPtr());
  }
  return uSurfaces;
}

}  // namespace

Acts::GeometryContext tContext;

auto cGeometry = Acts::Test::CylindricalTrackingGeometry(tContext);
auto nominal = Acts::Transform3::Identity();

BOOST_AUTO_TEST_SUITE(ActSvg)

BOOST_AUTO_TEST_CASE(TubeCylindricalDetectorVolume) {
  auto portalGenerator = Acts::Experimental::defaultPortalGenerator();

  // The volume definitions
  double rInner = 10.;
  double rOuter = 100.;
  double zHalfL = 300.;

  Acts::Svg::Style portalStyle;
  portalStyle.fillColor = {255, 255, 255};
  portalStyle.fillOpacity = 0.;

  // A tube cylinder
  auto tubeCylinderBounds =
      std::make_unique<Acts::CylinderVolumeBounds>(rInner, rOuter, zHalfL);

  auto tubeCylinderVolume =
      Acts::Experimental::DetectorVolumeFactory::construct(
          portalGenerator, tContext, "TubeCylinderVolume", nominal,
          std::move(tubeCylinderBounds), Acts::Experimental::tryAllPortals());

  Acts::Svg::DetectorVolumeConverter::Options volumeOptions;
  volumeOptions.portalOptions.volumeIndices[tubeCylinderVolume.get()] = 0u;

  auto [pVolume, pGrid] = Acts::Svg::DetectorVolumeConverter::convert(
      tContext, *tubeCylinderVolume, volumeOptions);
  pVolume._name = tubeCylinderVolume->name();

  // Colorize in red
  actsvg::style::color red({{255, 0, 0}});
  red._opacity = 0.1;
  std::vector<actsvg::style::color> colors = {red};
  pVolume.colorize(colors);

  // As sheet
  auto pv = Acts::Svg::View::zr(pVolume, pVolume._name);
  Acts::Svg::toFile({pv}, pVolume._name + "_zr.svg");
}

BOOST_AUTO_TEST_CASE(TubeSectorCylindricalDetectorVolume) {
  auto portalGenerator = Acts::Experimental::defaultPortalGenerator();

  // The volume definitions
  double rInner = 10.;
  double rOuter = 100.;
  double zHalfL = 300.;
  double phiSector = std::numbers::pi / 4.;
  std::vector<double> avgPhi = {0., 0.75};
  std::vector<std::string> avgPhiTag = {"zero", "nonzero"};

  Acts::Svg::Style portalStyle;
  portalStyle.fillColor = {255, 255, 255};
  portalStyle.fillOpacity = 0.;

  std::vector<actsvg::svg::object> volumesXY;
  for (auto [iphi, aphi] : Acts::enumerate(avgPhi)) {
    // A tube cylinder
    auto sectorCylinderBounds = std::make_unique<Acts::CylinderVolumeBounds>(
        rInner, rOuter, zHalfL, phiSector, aphi);

    auto sectorCylinderVolume =
        Acts::Experimental::DetectorVolumeFactory::construct(
            portalGenerator, tContext, "SectoralCylinderVolume", nominal,
            std::move(sectorCylinderBounds),
            Acts::Experimental::tryAllPortals());

    Acts::Svg::DetectorVolumeConverter::Options volumeOptions;
    volumeOptions.portalOptions.volumeIndices[sectorCylinderVolume.get()] = 0u;

    auto [pVolume, pGrid] = Acts::Svg::DetectorVolumeConverter::convert(
        tContext, *sectorCylinderVolume, volumeOptions);

    // Colorize in blue
    actsvg::style::color blue({{0, 0, 255}});
    blue._opacity = 0.1;
    std::vector<actsvg::style::color> colors = {blue};
    pVolume.colorize(colors);

    volumesXY.push_back(Acts::Svg::View::xy(pVolume, pVolume._name));
  }

  Acts::Svg::toFile(volumesXY, "SectorVolumes_xy.svg");
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
  Acts::Svg::DetectorVolumeConverter::Options volumeOptions;
  volumeOptions.portalOptions.volumeIndices[volume.get()] = 0u;

  Acts::Svg::SurfaceConverter::Options surfaceOptions;
  surfaceOptions.style.fillColor = {50, 121, 168};
  surfaceOptions.style.fillOpacity = 0.5;
  volumeOptions.surfaceOptions = surfaceOptions;

  auto [pVolume, pGrid] = Acts::Svg::DetectorVolumeConverter::convert(
      tContext, *volume, volumeOptions);

  // x-y view
  auto volumeXY = Acts::Svg::View::xy(pVolume, pVolume._name);
  Acts::Svg::toFile({volumeXY}, "EndcapVolume_xy.svg");

  // z-r view
  auto volumeZR = Acts::Svg::View::zr(pVolume, pVolume._name);
  Acts::Svg::toFile({volumeZR}, "EndcapVolume_zr.svg");

  // The grid surfaces
  auto gridXY = Acts::Svg::View::xy(pGrid, "EndcapVolume_grid_xy");
  Acts::Svg::toFile({gridXY}, "EndcapVolume_grid_xy.svg");
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
  dvCfg.name = "CylinderWithSurface";
  dvCfg.externalsBuilder = shapeBuilder;
  dvCfg.internalsBuilder = barrelBuilder;

  auto dvBuilder = std::make_shared<Acts::Experimental::DetectorVolumeBuilder>(
      dvCfg, Acts::getDefaultLogger("EndcapBuilder", Acts::Logging::VERBOSE));

  auto [volumes, portals, roots] = dvBuilder->construct(tContext);

  auto volume = volumes.front();
  Acts::Svg::DetectorVolumeConverter::Options volumeOptions;
  volumeOptions.portalOptions.volumeIndices[volume.get()] = 0u;

  Acts::Svg::SurfaceConverter::Options surfaceOptions;
  surfaceOptions.style.fillColor = {50, 121, 168};
  surfaceOptions.style.fillOpacity = 0.5;
  volumeOptions.surfaceOptions = surfaceOptions;

  auto [pVolume, pGrid] = Acts::Svg::DetectorVolumeConverter::convert(
      tContext, *volume, volumeOptions);

  // x-y view
  auto volumeXY = Acts::Svg::View::xy(pVolume, pVolume._name);
  Acts::Svg::toFile({volumeXY}, "BarrelVolume_xy.svg");

  // z-r view
  auto volumeZR = Acts::Svg::View::zr(pVolume, pVolume._name);
  Acts::Svg::toFile({volumeZR}, "BarrelVolume_zr.svg");

  // The grid surfaces
  auto gridZPhi = Acts::Svg::View::zphi(pGrid, "BarrelVolume_grid_zphi");
  Acts::Svg::toFile({gridZPhi}, "BarrelVolume_grid_zphi.svg");
}

BOOST_AUTO_TEST_SUITE_END()
