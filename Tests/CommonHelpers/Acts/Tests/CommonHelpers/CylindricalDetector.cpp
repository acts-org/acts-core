// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Tests/CommonHelpers/CylindricalDetector.hpp"

#include "Acts/Detector/CylindricalContainerBuilder.hpp"
#include "Acts/Detector/DetectorBuilder.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"

#include <memory>

auto materialSlab =
    Acts::MaterialSlab(Acts::Material::fromMolarDensity(1, 2, 3, 4, 5), 1.);

using namespace Acts;
using namespace Acts::Experimental;

std::shared_ptr<const Detector> Acts::Test::buildCylindricalDetector(
    const Acts::GeometryContext& tContext) {
  auto material =
      std::make_shared<const HomogeneousSurfaceMaterial>(materialSlab);

  auto beampipe = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(0., 50., 400.),
      CylinderBounds(25., 380.), "BeamPipe", material);

  // Declare a negative disc builder
  Transform3 negZ = Transform3::Identity();
  negZ.pretranslate(Vector3(0., 0., -300.));
  auto endcapN =
      std::make_shared<CylindricalVolumeBuilder<DiscSurface, RadialBounds>>(
          negZ, CylinderVolumeBounds(50., 140., 100.), RadialBounds(60., 120.),
          "NegativeEndcap", material);

  // Declare a barrel sub builder
  auto barrel0 = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(50., 80., 200.),
      CylinderBounds(65., 180.), "Barrel0", material);

  // Declare a barrel sub builder
  auto barrel1 = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(80., 110., 200.),
      CylinderBounds(95., 180.), "Barrel1", material);

  // Declare a barrel sub builder
  auto barrel2 = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(110., 140., 200.),
      CylinderBounds(125., 180.), "Barrel2", material);

  // Create the barrel container builder
  CylindricalContainerBuilder::Config barrelRCfg;
  barrelRCfg.builders = {barrel0, barrel1, barrel2};
  barrelRCfg.binning = {BinningValue::binR};

  auto barrel = std::make_shared<CylindricalContainerBuilder>(
      barrelRCfg, getDefaultLogger("BarrelBuilderR", Logging::INFO));

  Transform3 posZ = Transform3::Identity();
  posZ.pretranslate(Vector3(0., 0., 300.));
  auto endcapP =
      std::make_shared<CylindricalVolumeBuilder<DiscSurface, RadialBounds>>(
          posZ, CylinderVolumeBounds(50., 140., 100.), RadialBounds(60., 120.),
          "PositiveEndcap", material);

  // Create the barrel container builder
  CylindricalContainerBuilder::Config barrelEndcapCfg;
  barrelEndcapCfg.builders = {endcapN, barrel, endcapP};
  barrelEndcapCfg.binning = {BinningValue::binZ};

  auto barrelEndcap = std::make_shared<CylindricalContainerBuilder>(
      barrelEndcapCfg, getDefaultLogger("BarrelEndcapBuilder", Logging::INFO));

  // Create the barrel container builder
  CylindricalContainerBuilder::Config detectorCfg;
  detectorCfg.builders = {beampipe, barrelEndcap};
  detectorCfg.binning = {BinningValue::binR};

  auto containerBuilder = std::make_shared<CylindricalContainerBuilder>(
      detectorCfg, getDefaultLogger("DetectorBuilder", Logging::INFO));

  // Detector builder
  auto gigConfig = GeometryIdGenerator::Config();
  auto gig = std::make_shared<GeometryIdGenerator>(gigConfig);

  Acts::Experimental::DetectorBuilder::Config dCfg;
  dCfg.auxiliary = "*** Test : Cylindrical Detector ***";
  dCfg.name = "CylindricalDetector";
  dCfg.builder = containerBuilder;
  dCfg.geoIdGenerator = gig;

  return DetectorBuilder(dCfg).construct(tContext);
}
