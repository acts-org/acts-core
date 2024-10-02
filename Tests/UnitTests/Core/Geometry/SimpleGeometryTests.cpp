// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CylinderVolumeBuilder.hpp"
#include "Acts/Geometry/CylinderVolumeHelper.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/PassiveLayerBuilder.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingGeometryBuilder.hpp"
#include "Acts/Geometry/TrackingVolumeArrayCreator.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();

/// @brief Unit test for a three layer detector parameters
/// Testing the Tool chain in the geometry building process
///
BOOST_AUTO_TEST_CASE(SimpleGeometryTest) {
  Logging::Level surfaceLLevel = Logging::INFO;
  Logging::Level layerLLevel = Logging::INFO;
  Logging::Level volumeLLevel = Logging::INFO;

  // configure surface array creator
  SurfaceArrayCreator::Config sacConfig;
  auto surfaceArrayCreator = std::make_shared<const SurfaceArrayCreator>(
      sacConfig, getDefaultLogger("SurfaceArrayCreator", surfaceLLevel));
  // configure the layer creator that uses the surface array creator
  LayerCreator::Config lcConfig;
  lcConfig.surfaceArrayCreator = surfaceArrayCreator;
  auto layerCreator = std::make_shared<const LayerCreator>(
      lcConfig, getDefaultLogger("LayerCreator", layerLLevel));
  // configure the layer array creator
  LayerArrayCreator::Config lacConfig;
  auto layerArrayCreator = std::make_shared<const LayerArrayCreator>(
      lacConfig, getDefaultLogger("LayerArrayCreator", layerLLevel));

  // tracking volume array creator
  TrackingVolumeArrayCreator::Config tvacConfig;
  auto tVolumeArrayCreator = std::make_shared<const TrackingVolumeArrayCreator>(
      tvacConfig, getDefaultLogger("TrackingVolumeArrayCreator", volumeLLevel));
  // configure the cylinder volume helper
  CylinderVolumeHelper::Config cvhConfig;
  cvhConfig.layerArrayCreator = layerArrayCreator;
  cvhConfig.trackingVolumeArrayCreator = tVolumeArrayCreator;
  auto cylinderVolumeHelper = std::make_shared<const CylinderVolumeHelper>(
      cvhConfig, getDefaultLogger("CylinderVolumeHelper", volumeLLevel));

  // ----------------- build a beam pipe -----------------------------------
  PassiveLayerBuilder::Config bplConfig;
  bplConfig.layerIdentification = "BeamPipe";
  bplConfig.centralLayerRadii = std::vector<double>(1, 3_mm);
  bplConfig.centralLayerHalflengthZ = std::vector<double>(1, 40_mm);
  bplConfig.centralLayerThickness = std::vector<double>(1, 0.8_mm);
  auto beamPipeBuilder = std::make_shared<const PassiveLayerBuilder>(
      bplConfig, getDefaultLogger("BeamPipeLayerBuilder", layerLLevel));
  // create the volume for the beam pipe
  CylinderVolumeBuilder::Config bpvConfig;
  bpvConfig.trackingVolumeHelper = cylinderVolumeHelper;
  bpvConfig.volumeName = "BeamPipe";
  bpvConfig.layerBuilder = beamPipeBuilder;
  bpvConfig.layerEnvelopeR = {1_mm, 1_mm};
  bpvConfig.buildToRadiusZero = true;
  auto beamPipeVolumeBuilder = std::make_shared<const CylinderVolumeBuilder>(
      bpvConfig, getDefaultLogger("BeamPipeVolumeBuilder", volumeLLevel));

  PassiveLayerBuilder::Config layerBuilderConfig;
  layerBuilderConfig.layerIdentification = "CentralBarrel";
  layerBuilderConfig.centralLayerRadii = {10_mm, 20_mm, 30_mm};
  layerBuilderConfig.centralLayerHalflengthZ = {40_mm, 40_mm, 40_mm};
  layerBuilderConfig.centralLayerThickness = {1_mm, 1_mm, 1_mm};
  auto layerBuilder = std::make_shared<const PassiveLayerBuilder>(
      layerBuilderConfig,
      getDefaultLogger("CentralBarrelBuilder", layerLLevel));
  // create the volume for the central barrel
  CylinderVolumeBuilder::Config cvbConfig;
  cvbConfig.trackingVolumeHelper = cylinderVolumeHelper;
  cvbConfig.volumeName = "CentralBarrel";
  cvbConfig.layerBuilder = layerBuilder;
  cvbConfig.layerEnvelopeR = {1_mm, 1_mm};
  cvbConfig.buildToRadiusZero = false;
  auto centralVolumeBuilder = std::make_shared<const CylinderVolumeBuilder>(
      cvbConfig, getDefaultLogger("CentralVolumeBuilder", volumeLLevel));

  // Make the TrackingGeometry Builder
  TrackingGeometryBuilder::Config tgbConfig;
  tgbConfig.trackingVolumeBuilders.push_back(
      [=](const auto& context, const auto& inner, const auto&) {
        return beamPipeVolumeBuilder->trackingVolume(context, inner);
      });
  tgbConfig.trackingVolumeBuilders.push_back(
      [=](const auto& context, const auto& inner, const auto&) {
        return centralVolumeBuilder->trackingVolume(context, inner);
      });
  tgbConfig.trackingVolumeHelper = cylinderVolumeHelper;

  TrackingGeometryBuilder tgBuilder(tgbConfig);
  auto tGeometry = tgBuilder.trackingGeometry(tgContext);

  BOOST_CHECK(tGeometry != nullptr);
}
}  // namespace Acts::Test
