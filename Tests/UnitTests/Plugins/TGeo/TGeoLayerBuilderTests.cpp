// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/ProtoLayerHelper.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Plugins/TGeo/TGeoLayerBuilder.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Tests/CommonHelpers/DataDirectory.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "TGeoManager.h"

using namespace Acts::UnitLiterals;

namespace Acts::Test {

/// @brief struct to load the global geometry
struct RootGeometry {
  RootGeometry() {
    auto path = Acts::Test::getDataPath("panda.root");
    TGeoManager::Import(path.c_str());
  }
};

RootGeometry rGeometry = RootGeometry();

GeometryContext tgContext = GeometryContext();

/// @brief Unit test checking the match probability
BOOST_AUTO_TEST_CASE(TGeoLayerBuilderTests) {
  using TglConfig = TGeoLayerBuilder::LayerConfig;

  TglConfig b0Config;
  b0Config.volumeName = "*";
  b0Config.sensorNames = {"PixelActiveo2", "PixelActiveo4", "PixelActiveo5",
                          "PixelActiveo6"};
  b0Config.localAxes = "XYZ";
  b0Config.parseRanges = {{BinningValue::binR, {0., 40_mm}},
                          {BinningValue::binZ, {-60_mm, 15_mm}}};
  b0Config.envelope = {0_mm, 0_mm};

  TglConfig eAllConfig;
  eAllConfig.volumeName = "*";
  eAllConfig.sensorNames = {"PixelActiveo2", "PixelActiveo4", "PixelActiveo5",
                            "PixelActiveo6"};
  eAllConfig.localAxes = "XYZ";
  eAllConfig.parseRanges = {{BinningValue::binR, {0., 40_mm}},
                            {BinningValue::binZ, {16_mm, 60_mm}}};
  eAllConfig.splitConfigs = {{BinningValue::binZ, 5_mm}};
  eAllConfig.envelope = {0_mm, 0_mm};

  std::vector<TglConfig> cConfigs = {b0Config};
  std::vector<TglConfig> pConfigs = {eAllConfig};

  TGeoLayerBuilder::Config tglbConfig;
  tglbConfig.configurationName = "Pixels";
  tglbConfig.layerConfigurations[1] = cConfigs;
  tglbConfig.layerConfigurations[2] = pConfigs;

  auto surfaceArrayCreator = std::make_shared<const SurfaceArrayCreator>(
      getDefaultLogger("SurfaceArrayCreator", Logging::VERBOSE));

  LayerCreator::Config lcConfig;
  lcConfig.surfaceArrayCreator = surfaceArrayCreator;
  auto layerCreator = std::make_shared<const LayerCreator>(
      lcConfig, getDefaultLogger("LayerCreator", Logging::VERBOSE));
  tglbConfig.layerCreator = layerCreator;

  ProtoLayerHelper::Config plhConfig;
  auto protoLayerHelper = std::make_shared<const ProtoLayerHelper>(
      plhConfig, getDefaultLogger("ProtoLayerHelper", Logging::VERBOSE));
  tglbConfig.protoLayerHelper = protoLayerHelper;

  TGeoLayerBuilder tglb(tglbConfig,
                        getDefaultLogger("TGeoLayerBuilder", Logging::VERBOSE));

  ObjVisualization3D objVis;

  auto centralLayers = tglb.centralLayers(tgContext);
  BOOST_CHECK_EQUAL(centralLayers.size(), 1u);
  BOOST_CHECK_EQUAL(tglb.detectorElements().size(), 14u);

  auto positiveLayers = tglb.positiveLayers(tgContext);
  // Check that it's split into two layers
  std::size_t ipl = 0;
  BOOST_CHECK_EQUAL(positiveLayers.size(), 2u);
  BOOST_CHECK_EQUAL(tglb.detectorElements().size(), 14u + 16u);
  for (const auto& pLayer : positiveLayers) {
    auto sArray = pLayer->surfaceArray();
    if (sArray != nullptr) {
      for (auto& surface : sArray->surfaces()) {
        GeometryView3D::drawSurface(objVis, *surface, tgContext);
      }
    }
    objVis.write("PositiveLayer_" + std::to_string(ipl++));
    objVis.clear();
  }
}

}  // namespace Acts::Test
