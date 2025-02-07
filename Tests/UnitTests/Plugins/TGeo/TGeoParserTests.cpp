// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/TGeo/TGeoParser.hpp"
#include "Acts/Plugins/TGeo/TGeoSurfaceConverter.hpp"
#include "Acts/Tests/CommonHelpers/DataDirectory.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "TGeoManager.h"

namespace Acts::Test {

/// @brief struct to load the global geometry
struct RootGeometry {
  RootGeometry() {
    auto path = Acts::Test::getDataPath("panda.root");
    TGeoManager::Import(path.c_str());
  }
};

GeometryContext tgContext = GeometryContext();

RootGeometry rGeometry = RootGeometry();

/// @brief Unit test Parsing a TGeo geometry
BOOST_AUTO_TEST_CASE(TGeoParser_Pixel) {
  if (gGeoManager != nullptr) {
    std::string volumeName = "*";
    TGeoParser::Options tgpOptions;
    tgpOptions.volumeNames = {volumeName};
    tgpOptions.targetNames = {"PixelActiveo2", "PixelActiveo4", "PixelActiveo5",
                              "PixelActiveo6"};
    std::string axes = "XYZ";
    double scale = 10.;

    TGeoParser::State tgpState;
    tgpState.volume = gGeoManager->GetTopVolume();

    // Parse the full ones
    TGeoParser::select(tgpState, tgpOptions);

    // This should select 176 PixelActive modules
    BOOST_CHECK_EQUAL(tgpState.selectedNodes.size(), 176u);

    /// Convert into surfaces using the TGeoSurfaceConverter & Draw them
    ObjVisualization3D objVis;
    for (auto& snode : tgpState.selectedNodes) {
      const auto& shape = *(snode.node->GetVolume()->GetShape());
      const auto& transform = *(snode.transform.get());
      auto [surface, thickness] =
          TGeoSurfaceConverter::toSurface(shape, transform, axes, scale);
      GeometryView3D::drawSurface(objVis, *surface, tgContext);
    }
    objVis.write("PixelActive");
  }
}

/// @brief Unit test Parsing a TGeo geometries
BOOST_AUTO_TEST_CASE(TGeoParser_Pixel_SelectInnermost) {
  if (gGeoManager != nullptr) {
    std::string volumeName = "*";
    TGeoParser::Options tgpOptions;
    tgpOptions.volumeNames = {volumeName};
    tgpOptions.targetNames = {"PixelActiveo2", "PixelActiveo4", "PixelActiveo5",
                              "PixelActiveo6"};
    tgpOptions.parseRanges.push_back({AxisDirection::AxisR, {0., 40.}});
    tgpOptions.parseRanges.push_back({AxisDirection::AxisZ, {-60., 15.}});
    tgpOptions.unit = 10.;

    std::string axes = "XYZ";

    TGeoParser::State tgpState;
    tgpState.volume = gGeoManager->GetTopVolume();

    // Parse the full ones
    TGeoParser::select(tgpState, tgpOptions);

    // This should select 14 PixelActive modules
    BOOST_CHECK_EQUAL(tgpState.selectedNodes.size(), 14u);

    /// Convert into surfaces using the TGeoSurfaceConverter & Draw them
    ObjVisualization3D objVis;
    for (auto& snode : tgpState.selectedNodes) {
      const auto& shape = *(snode.node->GetVolume()->GetShape());
      const auto& transform = *(snode.transform.get());
      auto [surface, thickness] = TGeoSurfaceConverter::toSurface(
          shape, transform, axes, tgpOptions.unit);
      GeometryView3D::drawSurface(objVis, *surface, tgContext);
    }
    objVis.write("PixelActive_Innermost");
  }
}

}  // namespace Acts::Test
