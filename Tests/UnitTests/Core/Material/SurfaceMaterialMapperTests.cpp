// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBuilder.hpp"
#include "Acts/Geometry/CylinderVolumeHelper.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/PassiveLayerBuilder.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolumeArrayCreator.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Material/SurfaceMaterialMapper.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Acts {

/// @brief create a small tracking geometry to map some dummy material on
std::shared_ptr<const TrackingGeometry> trackingGeometry() {
  using namespace Acts::UnitLiterals;

  BinUtility zbinned(8, -40, 40, open, BinningValue::binZ);
  auto matProxy = std::make_shared<const ProtoSurfaceMaterial>(zbinned);

  Logging::Level surfaceLLevel = Logging::INFO;
  Logging::Level layerLLevel = Logging::INFO;
  Logging::Level volumeLLevel = Logging::INFO;

  // configure surface array creator
  auto surfaceArrayCreator = std::make_shared<const SurfaceArrayCreator>(
      getDefaultLogger("SurfaceArrayCreator", surfaceLLevel));
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

  PassiveLayerBuilder::Config layerBuilderConfig;
  layerBuilderConfig.layerIdentification = "CentralBarrel";
  layerBuilderConfig.centralLayerRadii = {10., 20., 30.};
  layerBuilderConfig.centralLayerHalflengthZ = {40., 40., 40.};
  layerBuilderConfig.centralLayerThickness = {1., 1., 1.};
  layerBuilderConfig.centralLayerMaterial = {matProxy, matProxy, matProxy};
  auto layerBuilder = std::make_shared<const PassiveLayerBuilder>(
      layerBuilderConfig,
      getDefaultLogger("CentralBarrelBuilder", layerLLevel));
  // create the volume for the beam pipe
  CylinderVolumeBuilder::Config cvbConfig;
  cvbConfig.trackingVolumeHelper = cylinderVolumeHelper;
  cvbConfig.volumeName = "BeamPipe";
  cvbConfig.layerBuilder = layerBuilder;
  cvbConfig.layerEnvelopeR = {1_mm, 1_mm};
  cvbConfig.buildToRadiusZero = true;
  auto centralVolumeBuilder = std::make_shared<const CylinderVolumeBuilder>(
      cvbConfig, getDefaultLogger("CentralVolumeBuilder", volumeLLevel));

  // create the bounds and the volume
  auto centralVolumeBounds =
      std::make_shared<const CylinderVolumeBounds>(0., 40., 110.);

  GeometryContext gCtx;
  auto centralVolume =
      centralVolumeBuilder->trackingVolume(gCtx, nullptr, centralVolumeBounds);

  return std::make_shared<const TrackingGeometry>(centralVolume);
}

std::shared_ptr<const TrackingGeometry> tGeometry = trackingGeometry();

}  // namespace Acts

namespace Acts::Test {

/// Test the filling and conversion
BOOST_AUTO_TEST_CASE(SurfaceMaterialMapper_tests) {
  /// We need a Navigator, Stepper to build a Propagator
  Navigator navigator({tGeometry});
  StraightLineStepper stepper;
  SurfaceMaterialMapper::StraightLinePropagator propagator(
      stepper, std::move(navigator));

  /// The config object
  SurfaceMaterialMapper::Config smmConfig;
  SurfaceMaterialMapper smMapper(smmConfig, std::move(propagator));

  /// Create some contexts
  GeometryContext gCtx;
  MagneticFieldContext mfCtx;

  /// Now create the mapper state
  auto mState = smMapper.createState(gCtx, mfCtx, *tGeometry);

  /// Test if this is not null
  BOOST_CHECK_EQUAL(mState.accumulatedMaterial.size(), 3u);
}

}  // namespace Acts::Test
