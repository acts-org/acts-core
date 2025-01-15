// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/AccumulatedVolumeMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialGridHelper.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Grid.hpp"

#include <cmath>
#include <functional>
#include <memory>
#include <numbers>
#include <utility>
#include <vector>

namespace Acts::Test {

using EAxis = Acts::Axis<AxisType::Equidistant>;
using Grid2D = Acts::Grid<Acts::AccumulatedVolumeMaterial, EAxis, EAxis>;
using Grid3D = Acts::Grid<Acts::AccumulatedVolumeMaterial, EAxis, EAxis, EAxis>;
using MaterialGrid2D =
    Acts::Grid<Acts::Material::ParametersVector, EAxis, EAxis>;
using MaterialGrid3D =
    Acts::Grid<Acts::Material::ParametersVector, EAxis, EAxis, EAxis>;

/// @brief Various test for the Material in the case of a Cuboid volume and 2D
/// Grid
BOOST_AUTO_TEST_CASE(Square_Grid_test) {
  BinUtility bu(7, -3., 3., open, AxisDirection::AxisX);
  bu += BinUtility(3, -2., 2., open, AxisDirection::AxisY);
  auto bd = bu.binningData();
  std::function<Acts::Vector2(Acts::Vector3)> transfoGlobalToLocal;

  Grid2D Grid = createGrid2D(bu, transfoGlobalToLocal);

  // Test Global To Local transform
  Acts::Vector3 pos(1., 2., 3.);
  Acts::Vector2 pos_2d(1., 2.);
  BOOST_CHECK_EQUAL(pos_2d, transfoGlobalToLocal(pos));

  // Test Grid
  BOOST_CHECK_EQUAL(Grid.dimensions(), 2);

  BOOST_CHECK_EQUAL(Grid.numLocalBins()[0], bd[0].bins());
  BOOST_CHECK_EQUAL(Grid.numLocalBins()[1], bd[1].bins());

  BOOST_CHECK_EQUAL(Grid.minPosition()[0], bd[0].min);
  BOOST_CHECK_EQUAL(Grid.minPosition()[1], bd[1].min);

  float max1 = bd[0].max + std::abs(bd[0].max - bd[0].min) / (bd[0].bins() - 1);
  float max2 = bd[1].max + std::abs(bd[1].max - bd[1].min) / (bd[1].bins() - 1);

  BOOST_CHECK_EQUAL(Grid.maxPosition()[0], max1);
  BOOST_CHECK_EQUAL(Grid.maxPosition()[1], max2);

  // Test pos to index
  Grid2D::index_t index1 = {1, 1};
  Grid2D::index_t index2 = {7, 2};
  Grid2D::index_t index3 = {1, 3};

  Acts::Vector3 pos1 = {-2.6, -1.5, -0.7};
  Acts::Vector3 pos2 = {2.8, 0, 0.2};
  Acts::Vector3 pos3 = {-2.7, 1.8, 0.8};

  for (int i = 0; i < 2; i++) {
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos1))[i],
        index1[i]);
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos2))[i],
        index2[i]);
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos3))[i],
        index3[i]);
  }
  // Test material mapping

  std::vector<Acts::Vector3> vectPos1;
  vectPos1.push_back(pos1);
  std::vector<Acts::Vector3> vectPos2;
  vectPos2.push_back(pos2);
  std::vector<Acts::Vector3> vectPos3;
  vectPos3.push_back(pos3);

  std::vector<std::pair<MaterialSlab, std::vector<Vector3>>> matRecord;
  Material mat1 = Material::fromMolarDensity(1., 2., 3., 4., 5.);
  Material mat2 = Material::fromMolarDensity(6., 7., 8., 9., 10.);
  Material vacuum;

  MaterialSlab matprop1(mat1, 1);
  MaterialSlab matprop2(mat2, 1);

  matRecord.clear();
  matRecord.push_back(std::make_pair(matprop1, vectPos1));
  matRecord.push_back(std::make_pair(matprop2, vectPos2));

  // Walk over each property
  for (const auto& rm : matRecord) {
    // Walk over each point associated with the properties
    for (const auto& point : rm.second) {
      // Search for fitting grid point and accumulate
      Acts::Grid2D::index_t index =
          Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(point));
      Grid.atLocalBins(index).accumulate(rm.first);
    }
  }

  MaterialGrid2D matMap = mapMaterialPoints(Grid);

  CHECK_CLOSE_REL(matMap.atLocalBins(index1), mat1.parameters(), 1e-4);
  CHECK_CLOSE_REL(matMap.atLocalBins(index2), mat2.parameters(), 1e-4);
  BOOST_CHECK_EQUAL(matMap.atLocalBins(index3), vacuum.parameters());
}

/// @brief Various test for the Material in the case of a Cylindrical volume
/// with a 2D grid
BOOST_AUTO_TEST_CASE(PhiZ_Grid_test) {
  BinUtility bu(2, -2., 2., open, AxisDirection::AxisZ);
  bu += BinUtility(3, -std::numbers::pi, std::numbers::pi, closed,
                   AxisDirection::AxisPhi);
  auto bd = bu.binningData();
  std::function<Acts::Vector2(Acts::Vector3)> transfoGlobalToLocal;

  Grid2D Grid = createGrid2D(bu, transfoGlobalToLocal);

  // Test Global To Local transform
  Acts::Vector3 pos(1., 2., 3.);

  CHECK_CLOSE_REL(transfoGlobalToLocal(pos)[1], atan2(2, 1), 1e-4);
  CHECK_CLOSE_REL(transfoGlobalToLocal(pos)[0], 3, 1e-4);

  // Test Grid
  BOOST_CHECK_EQUAL(Grid.dimensions(), 2);

  BOOST_CHECK_EQUAL(Grid.numLocalBins()[0], bd[0].bins());
  BOOST_CHECK_EQUAL(Grid.numLocalBins()[1], bd[1].bins());

  BOOST_CHECK_EQUAL(Grid.minPosition()[0], bd[0].min);
  BOOST_CHECK_EQUAL(Grid.minPosition()[1], bd[1].min);

  float max1 = bd[0].max + std::abs(bd[0].max - bd[0].min) / (bd[0].bins() - 1);
  float max2 = bd[1].max + std::abs(bd[1].max - bd[1].min) / (bd[1].bins() - 1);

  BOOST_CHECK_EQUAL(Grid.maxPosition()[0], max1);
  BOOST_CHECK_EQUAL(Grid.maxPosition()[1], max2);

  // Test pos to index
  Grid2D::index_t index1 = {1, 1};
  Grid2D::index_t index2 = {1, 2};
  Grid2D::index_t index3 = {2, 3};

  Acts::Vector3 pos1 = {-0.2, -1, -1};
  Acts::Vector3 pos2 = {3.6, 0., -1.5};
  Acts::Vector3 pos3 = {-1, 0.3, 0.8};

  for (int i = 0; i < 2; i++) {
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos1))[i],
        index1[i]);
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos2))[i],
        index2[i]);
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos3))[i],
        index3[i]);
  }

  // Test material mapping
  std::vector<Acts::Vector3> vectPos1;
  vectPos1.push_back(pos1);
  std::vector<Acts::Vector3> vectPos2;
  vectPos2.push_back(pos2);
  std::vector<Acts::Vector3> vectPos3;
  vectPos3.push_back(pos3);

  std::vector<std::pair<MaterialSlab, std::vector<Vector3>>> matRecord;
  Material mat1 = Material::fromMolarDensity(1., 2., 3., 4., 5.);
  Material mat2 = Material::fromMolarDensity(6., 7., 8., 9., 10.);
  Material vacuum;

  MaterialSlab matprop1(mat1, 1);
  MaterialSlab matprop2(mat2, 1);

  matRecord.clear();
  matRecord.push_back(std::make_pair(matprop1, vectPos1));
  matRecord.push_back(std::make_pair(matprop2, vectPos2));

  // Walk over each property
  for (const auto& rm : matRecord) {
    // Walk over each point associated with the properties
    for (const auto& point : rm.second) {
      // Search for fitting grid point and accumulate
      Acts::Grid2D::index_t index =
          Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(point));
      Grid.atLocalBins(index).accumulate(rm.first);
    }
  }

  MaterialGrid2D matMap = mapMaterialPoints(Grid);

  CHECK_CLOSE_REL(matMap.atLocalBins(index1), mat1.parameters(), 1e-4);
  CHECK_CLOSE_REL(matMap.atLocalBins(index2), mat2.parameters(), 1e-4);
  BOOST_CHECK_EQUAL(matMap.atLocalBins(index3), vacuum.parameters());
}

/// @brief Various test for the Material in the case of a Cuboid volume
BOOST_AUTO_TEST_CASE(Cubic_Grid_test) {
  BinUtility bu(7, -3., 3., open, AxisDirection::AxisX);
  bu += BinUtility(3, -2., 2., open, AxisDirection::AxisY);
  bu += BinUtility(2, -1., 1., open, AxisDirection::AxisZ);
  auto bd = bu.binningData();
  std::function<Acts::Vector3(Acts::Vector3)> transfoGlobalToLocal;

  Grid3D Grid = createGrid3D(bu, transfoGlobalToLocal);

  // Test Global To Local transform
  Acts::Vector3 pos(1., 2., 3.);
  BOOST_CHECK_EQUAL(pos, transfoGlobalToLocal(pos));

  // Test Grid
  BOOST_CHECK_EQUAL(Grid.dimensions(), 3);

  BOOST_CHECK_EQUAL(Grid.numLocalBins()[0], bd[0].bins());
  BOOST_CHECK_EQUAL(Grid.numLocalBins()[1], bd[1].bins());
  BOOST_CHECK_EQUAL(Grid.numLocalBins()[2], bd[2].bins());

  BOOST_CHECK_EQUAL(Grid.minPosition()[0], bd[0].min);
  BOOST_CHECK_EQUAL(Grid.minPosition()[1], bd[1].min);
  BOOST_CHECK_EQUAL(Grid.minPosition()[2], bd[2].min);

  float max1 = bd[0].max + std::abs(bd[0].max - bd[0].min) / (bd[0].bins() - 1);
  float max2 = bd[1].max + std::abs(bd[1].max - bd[1].min) / (bd[1].bins() - 1);
  float max3 = bd[2].max + std::abs(bd[2].max - bd[2].min) / (bd[2].bins() - 1);

  BOOST_CHECK_EQUAL(Grid.maxPosition()[0], max1);
  BOOST_CHECK_EQUAL(Grid.maxPosition()[1], max2);
  BOOST_CHECK_EQUAL(Grid.maxPosition()[2], max3);

  // Test pos to index
  Grid3D::index_t index1 = {1, 1, 1};
  Grid3D::index_t index2 = {7, 2, 2};
  Grid3D::index_t index3 = {1, 3, 2};

  Acts::Vector3 pos1 = {-2.6, -1.5, -0.7};
  Acts::Vector3 pos2 = {2.8, 0, 0.2};
  Acts::Vector3 pos3 = {-2.7, 1.8, 0.8};

  for (int i = 0; i < 3; i++) {
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos1))[i],
        index1[i]);
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos2))[i],
        index2[i]);
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos3))[i],
        index3[i]);
  }
  // Test material mapping
  std::vector<Acts::Vector3> vectPos1;
  vectPos1.push_back(pos1);
  std::vector<Acts::Vector3> vectPos2;
  vectPos2.push_back(pos2);
  std::vector<Acts::Vector3> vectPos3;
  vectPos3.push_back(pos3);

  std::vector<std::pair<MaterialSlab, std::vector<Vector3>>> matRecord;
  Material mat1 = Material::fromMolarDensity(1., 2., 3., 4., 5.);
  Material mat2 = Material::fromMolarDensity(6., 7., 8., 9., 10.);
  Material vacuum;

  MaterialSlab matprop1(mat1, 1);
  MaterialSlab matprop2(mat2, 1);

  matRecord.clear();
  matRecord.push_back(std::make_pair(matprop1, vectPos1));
  matRecord.push_back(std::make_pair(matprop2, vectPos2));

  // Walk over each property
  for (const auto& rm : matRecord) {
    // Walk over each point associated with the properties
    for (const auto& point : rm.second) {
      // Search for fitting grid point and accumulate
      Acts::Grid3D::index_t index =
          Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(point));
      Grid.atLocalBins(index).accumulate(rm.first);
    }
  }

  MaterialGrid3D matMap = mapMaterialPoints(Grid);

  CHECK_CLOSE_REL(matMap.atLocalBins(index1), mat1.parameters(), 1e-4);
  CHECK_CLOSE_REL(matMap.atLocalBins(index2), mat2.parameters(), 1e-4);
  BOOST_CHECK_EQUAL(matMap.atLocalBins(index3), vacuum.parameters());
}

/// @brief Various test for the Material in the case of a Cylindrical volume
BOOST_AUTO_TEST_CASE(Cylindrical_Grid_test) {
  BinUtility bu(4, 1., 4., open, AxisDirection::AxisR);
  bu += BinUtility(3, -std::numbers::pi, std::numbers::pi, closed,
                   AxisDirection::AxisPhi);
  bu += BinUtility(2, -2., 2., open, AxisDirection::AxisZ);
  auto bd = bu.binningData();
  std::function<Acts::Vector3(Acts::Vector3)> transfoGlobalToLocal;

  Grid3D Grid = createGrid3D(bu, transfoGlobalToLocal);

  // Test Global To Local transform
  Acts::Vector3 pos(1., 2., 3.);

  CHECK_CLOSE_REL(transfoGlobalToLocal(pos)[0], sqrt(5), 1e-4);
  CHECK_CLOSE_REL(transfoGlobalToLocal(pos)[1], atan2(2, 1), 1e-4);
  CHECK_CLOSE_REL(transfoGlobalToLocal(pos)[2], 3, 1e-4);

  // Test Grid
  BOOST_CHECK_EQUAL(Grid.dimensions(), 3);

  BOOST_CHECK_EQUAL(Grid.numLocalBins()[0], bd[0].bins());
  BOOST_CHECK_EQUAL(Grid.numLocalBins()[1], bd[1].bins());
  BOOST_CHECK_EQUAL(Grid.numLocalBins()[2], bd[2].bins());

  BOOST_CHECK_EQUAL(Grid.minPosition()[0], bd[0].min);
  BOOST_CHECK_EQUAL(Grid.minPosition()[1], bd[1].min);
  BOOST_CHECK_EQUAL(Grid.minPosition()[2], bd[2].min);

  float max1 = bd[0].max + std::abs(bd[0].max - bd[0].min) / (bd[0].bins() - 1);
  float max2 = bd[1].max + std::abs(bd[1].max - bd[1].min) / (bd[1].bins() - 1);
  float max3 = bd[2].max + std::abs(bd[2].max - bd[2].min) / (bd[2].bins() - 1);

  BOOST_CHECK_EQUAL(Grid.maxPosition()[0], max1);
  BOOST_CHECK_EQUAL(Grid.maxPosition()[1], max2);
  BOOST_CHECK_EQUAL(Grid.maxPosition()[2], max3);

  // Test pos to index
  Grid3D::index_t index1 = {1, 1, 1};
  Grid3D::index_t index2 = {4, 2, 1};
  Grid3D::index_t index3 = {1, 3, 2};

  Acts::Vector3 pos1 = {-0.2, -1, -1};
  Acts::Vector3 pos2 = {3.6, 0., -1.5};
  Acts::Vector3 pos3 = {-1, 0.3, 0.8};

  for (int i = 0; i < 3; i++) {
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos1))[i],
        index1[i]);
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos2))[i],
        index2[i]);
    BOOST_CHECK_EQUAL(
        Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(pos3))[i],
        index3[i]);
  }

  // Test material mapping
  std::vector<Acts::Vector3> vectPos1;
  vectPos1.push_back(pos1);
  std::vector<Acts::Vector3> vectPos2;
  vectPos2.push_back(pos2);
  std::vector<Acts::Vector3> vectPos3;
  vectPos3.push_back(pos3);

  std::vector<std::pair<MaterialSlab, std::vector<Vector3>>> matRecord;
  Material mat1 = Material::fromMolarDensity(1., 2., 3., 4., 5.);
  Material mat2 = Material::fromMolarDensity(6., 7., 8., 9., 10.);
  Material vacuum;

  MaterialSlab matprop1(mat1, 1);
  MaterialSlab matprop2(mat2, 1);

  matRecord.clear();
  matRecord.push_back(std::make_pair(matprop1, vectPos1));
  matRecord.push_back(std::make_pair(matprop2, vectPos2));

  // Walk over each property
  for (const auto& rm : matRecord) {
    // Walk over each point associated with the properties
    for (const auto& point : rm.second) {
      // Search for fitting grid point and accumulate
      Acts::Grid3D::index_t index =
          Grid.localBinsFromLowerLeftEdge(transfoGlobalToLocal(point));
      Grid.atLocalBins(index).accumulate(rm.first);
    }
  }

  MaterialGrid3D matMap = mapMaterialPoints(Grid);

  CHECK_CLOSE_REL(matMap.atLocalBins(index1), mat1.parameters(), 1e-4);
  CHECK_CLOSE_REL(matMap.atLocalBins(index2), mat2.parameters(), 1e-4);
  BOOST_CHECK_EQUAL(matMap.atLocalBins(index3), vacuum.parameters());
}

}  // namespace Acts::Test
