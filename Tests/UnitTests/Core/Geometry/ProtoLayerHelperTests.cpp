// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Geometry/ProtoLayerHelper.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/ViewConfig.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Acts {
class Surface;
}  // namespace Acts

namespace Acts::Test::Layers {

BOOST_AUTO_TEST_SUITE(Geometry)

BOOST_AUTO_TEST_CASE(ProtoLayerHelperTests) {
  ProtoLayerHelper::Config plhConfig;
  ProtoLayerHelper plHelper(
      plhConfig, getDefaultLogger("ProtoLayerHelper", Logging::VERBOSE));

  GeometryContext tgContext = GeometryContext();

  ObjVisualization3D objVis;

  CylindricalTrackingGeometry ctGeometry(tgContext);
  CylindricalTrackingGeometry::DetectorStore dStore;

  /// Cylindrical section ---------------------------------------------------
  std::vector<double> layerRadii = {32., 72., 116., 172.};
  std::vector<std::pair<int, int>> layerBinning = {
      {16, 14}, {32, 14}, {52, 14}, {78, 14}};
  std::vector<double> moduleTiltPhi = {0.145, 0.145, 0.145, 0.145};
  std::vector<double> moduleHalfX = {8.4, 8.4, 8.4, 8.4};
  std::vector<double> moduleHalfY = {36., 36., 36., 36.};
  std::vector<double> moduleThickness = {0.15, 0.15, 0.15, 0.15};

  std::vector<const Surface*> cylinderSurfaces;
  for (std::size_t ilp = 0; ilp < layerRadii.size(); ++ilp) {
    std::vector<const Surface*> layerSurfaces = ctGeometry.surfacesCylinder(
        dStore, moduleHalfX[ilp], moduleHalfY[ilp], moduleThickness[ilp],
        moduleTiltPhi[ilp], layerRadii[ilp], 2., 5., layerBinning[ilp]);
    cylinderSurfaces.insert(cylinderSurfaces.begin(), layerSurfaces.begin(),
                            layerSurfaces.end());
  }

  ViewConfig unsorted{.color = {252, 160, 0}};
  for (auto& sf : cylinderSurfaces) {
    GeometryView3D::drawSurface(objVis, *sf, tgContext, Transform3::Identity(),
                                unsorted);
  }
  // Draw the all surfaces
  objVis.write("ProtoLayerHelper_CylinderLayers_unsorted");
  objVis.clear();

  // Sort into ProtoLayers
  auto radialLayers = plHelper.protoLayers(
      tgContext, cylinderSurfaces,
      ProtoLayerHelper::SortingConfig(BinningValue::binR, 5.));

  BOOST_CHECK_EQUAL(radialLayers.size(), 4);

  std::vector<Color> sortedColors = {{102, 204, 255},
                                     {102, 255, 153},
                                     {255, 204, 102},
                                     {204, 102, 0},
                                     {278, 123, 55}};

  std::size_t il = 0;
  for (auto& layer : radialLayers) {
    for (auto& sf : layer.surfaces()) {
      ViewConfig sorted{.color = sortedColors[il]};
      GeometryView3D::drawSurface(objVis, *sf, tgContext,
                                  Transform3::Identity(), sorted);
    }
    ++il;
  }

  // Draw the sorted surfaces
  objVis.write("ProtoLayerHelper_CylinderLayers_radially");
  objVis.clear();

  /// Disc section ---------------------------------------------------
  std::vector<const Surface*> discSurfaces;

  std::vector<double> discZ = {-350., -250., -150., -100.};
  std::vector<double> discRadii = {55., 55., 55., 55.};
  std::vector<int> discModules = {22, 22, 22, 22};

  std::vector<double> dModuleHalfXMinY = {6.4, 6.4, 6.4, 6.4};
  std::vector<double> dModuleHalfXMaxY = {12.4, 12.4, 12.4, 12.4};
  std::vector<double> dModuleHalfY = {36., 36., 36., 36.};
  std::vector<double> dModuleTilt = {0.075, 0.075, 0.075, 0.075};
  std::vector<double> dModuleThickness = {0.15, 0.15, 0.15, 0.15};

  for (std::size_t ilp = 0; ilp < discZ.size(); ++ilp) {
    std::vector<const Surface*> layerSurfaces = ctGeometry.surfacesRing(
        dStore, dModuleHalfXMinY[ilp], dModuleHalfXMaxY[ilp], dModuleHalfY[ilp],
        dModuleThickness[ilp], dModuleTilt[ilp], discRadii[ilp], discZ[ilp], 2.,
        discModules[ilp]);
    discSurfaces.insert(discSurfaces.begin(), layerSurfaces.begin(),
                        layerSurfaces.end());
  }

  for (auto& sf : discSurfaces) {
    GeometryView3D::drawSurface(objVis, *sf, tgContext);
  }
  // Draw the all surfaces
  objVis.write("ProtoLayerHelper_DiscLayers_unsorted");
  objVis.clear();

  // Sort into ProtoLayers
  auto discLayersZ =
      plHelper.protoLayers(tgContext, discSurfaces, {BinningValue::binZ, 5.});

  BOOST_CHECK_EQUAL(discLayersZ.size(), 4);

  il = 0;
  for (auto& layer : discLayersZ) {
    for (auto& sf : layer.surfaces()) {
      ViewConfig ViewConfig{.color = sortedColors[il]};
      GeometryView3D::drawSurface(objVis, *sf, tgContext,
                                  Transform3::Identity(), ViewConfig);
    }
    ++il;
  }

  // Draw the sorted surfaces
  objVis.write("ProtoLayerHelper_DiscLayers_longitudinally");
  objVis.clear();

  /// Ring layout section ---------------------------------------------------
  std::vector<const Surface*> ringSurfaces;

  std::vector<double> ringZ = {-350., -250., -150., -100., -360., -255.,
                               -120., -330., -260., -150., -95.};
  std::vector<double> ringRadii = {32., 32., 32., 32., 58., 58.,
                                   58., 84., 84., 84., 84.};
  std::vector<int> ringModules = {22, 22, 22, 22, 32, 32, 32, 44, 44, 44, 44};

  std::vector<double> rModuleHalfXMinY(11, 6.4);
  std::vector<double> rModuleHalfXMaxY(11, 6.4);
  std::vector<double> rModuleHalfY(11, 10.);
  std::vector<double> rModuleTilt(11, 0.075);
  std::vector<double> rModuleThickness(11, 0.15);

  for (std::size_t ilp = 0; ilp < ringZ.size(); ++ilp) {
    std::vector<const Surface*> layerSurfaces = ctGeometry.surfacesRing(
        dStore, rModuleHalfXMinY[ilp], rModuleHalfXMaxY[ilp], rModuleHalfY[ilp],
        rModuleThickness[ilp], rModuleTilt[ilp], ringRadii[ilp], ringZ[ilp], 2.,
        ringModules[ilp]);
    ringSurfaces.insert(ringSurfaces.begin(), layerSurfaces.begin(),
                        layerSurfaces.end());
  }

  for (auto& sf : ringSurfaces) {
    GeometryView3D::drawSurface(objVis, *sf, tgContext);
  }
  // Draw the all surfaces
  objVis.write("ProtoLayerHelper_RingLayers_unsorted");
  objVis.clear();

  // First: Sort into ProtoLayers radially
  auto rSorted = plHelper.protoLayers(
      tgContext, ringSurfaces,
      ProtoLayerHelper::SortingConfig(BinningValue::binR, 1.));
  BOOST_CHECK_EQUAL(rSorted.size(), 3);

  Color dColor = {0, 0, 0};

  int ir = 0;
  for (auto& rBatch : rSorted) {
    auto lSorted = plHelper.protoLayers(
        tgContext, rBatch.surfaces(),
        ProtoLayerHelper::SortingConfig(BinningValue::binZ, 5.));
    il = 0;
    dColor[ir] = 256;
    for (auto& layer : lSorted) {
      dColor[ir] -= il * 50;
      for (auto& sf : layer.surfaces()) {
        GeometryView3D::drawSurface(objVis, *sf, tgContext);
      }
      ++il;
    }
    ++ir;
  }
  // Draw the all surfaces
  objVis.write("ProtoLayerHelper_RingLayers_sorted");

  // Perform the split at once
  auto rzSorted =
      plHelper.protoLayers(tgContext, ringSurfaces,
                           {{BinningValue::binR, 1.}, {BinningValue::binZ, 5}});

  std::size_t irz = 0;
  for (auto& layer : rzSorted) {
    for (auto& sf : layer.surfaces()) {
      GeometryView3D::drawSurface(objVis, *sf, tgContext);
    }
    objVis.write("ProtoLayerHelper_RingLayers_rz_sorted" +
                 std::to_string(irz++));
    objVis.clear();
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test::Layers
