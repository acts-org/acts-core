// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/MaterialGridHelper.hpp"

#include "Acts/Utilities/AlgebraHelpers.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

Acts::Grid2D Acts::createGrid(Acts::MaterialGridAxisData gridAxis1,
                              Acts::MaterialGridAxisData gridAxis2) {
  // get the number of bins
  std::size_t nBinsAxis1 = std::get<2>(gridAxis1);
  std::size_t nBinsAxis2 = std::get<2>(gridAxis2);

  // get the minimum and maximum
  double minAxis1 = std::get<0>(gridAxis1);
  double minAxis2 = std::get<0>(gridAxis2);
  double maxAxis1 = std::get<1>(gridAxis1);
  double maxAxis2 = std::get<1>(gridAxis2);
  // calculate maxima (add one last bin, because bin value always corresponds
  // to
  // left boundary)
  double stepAxis1 = std::abs(maxAxis1 - minAxis1) / (nBinsAxis1 - 1);
  double stepAxis2 = std::abs(maxAxis2 - minAxis2) / (nBinsAxis2 - 1);
  maxAxis1 += stepAxis1;
  maxAxis2 += stepAxis2;

  // Create the axis for the grid
  Acts::EAxis axis1(minAxis1, maxAxis1, nBinsAxis1);
  Acts::EAxis axis2(minAxis2, maxAxis2, nBinsAxis2);

  // The material mapping grid
  return Acts::Grid2D(std::make_tuple(std::move(axis1), std::move(axis2)));
}

Acts::Grid3D Acts::createGrid(Acts::MaterialGridAxisData gridAxis1,
                              Acts::MaterialGridAxisData gridAxis2,
                              Acts::MaterialGridAxisData gridAxis3) {
  // get the number of bins
  std::size_t nBinsAxis1 = std::get<2>(gridAxis1);
  std::size_t nBinsAxis2 = std::get<2>(gridAxis2);
  std::size_t nBinsAxis3 = std::get<2>(gridAxis3);

  // get the minimum and maximum
  double minAxis1 = std::get<0>(gridAxis1);
  double minAxis2 = std::get<0>(gridAxis2);
  double minAxis3 = std::get<0>(gridAxis3);
  double maxAxis1 = std::get<1>(gridAxis1);
  double maxAxis2 = std::get<1>(gridAxis2);
  double maxAxis3 = std::get<1>(gridAxis3);
  // calculate maxima (add one last bin, because bin value always corresponds
  // to
  // left boundary)
  double stepAxis1 =
      std::abs(maxAxis1 - minAxis1) / std::max(nBinsAxis1 - 1, std::size_t{1});
  double stepAxis2 =
      std::abs(maxAxis2 - minAxis2) / std::max(nBinsAxis2 - 1, std::size_t{1});
  double stepAxis3 =
      std::abs(maxAxis3 - minAxis3) / std::max(nBinsAxis3 - 1, std::size_t{1});
  maxAxis1 += stepAxis1;
  maxAxis2 += stepAxis2;
  maxAxis3 += stepAxis3;

  // Create the axis for the grid
  Acts::EAxis axis1(minAxis1, maxAxis1, nBinsAxis1);
  Acts::EAxis axis2(minAxis2, maxAxis2, nBinsAxis2);
  Acts::EAxis axis3(minAxis3, maxAxis3, nBinsAxis3);

  // The material mapping grid
  return Acts::Grid3D(
      std::make_tuple(std::move(axis1), std::move(axis2), std::move(axis3)));
}

std::function<double(Acts::Vector3)> Acts::globalToLocalFromBin(
    Acts::BinningValue& type) {
  std::function<double(Acts::Vector3)> transfoGlobalToLocal;

  switch (type) {
    case Acts::BinningValue::binX:
      transfoGlobalToLocal = [](const Acts::Vector3& pos) -> double {
        return (pos.x());
      };
      break;

    case Acts::BinningValue::binY:
      transfoGlobalToLocal = [](const Acts::Vector3& pos) -> double {
        return (pos.y());
      };
      break;

    case Acts::BinningValue::binR:
      transfoGlobalToLocal = [](const Acts::Vector3& pos) -> double {
        return (Acts::VectorHelpers::perp(pos));
      };
      break;

    case Acts::BinningValue::binPhi:
      transfoGlobalToLocal = [](const Acts::Vector3& pos) -> double {
        return (Acts::VectorHelpers::phi(pos));
      };
      break;

    case Acts::BinningValue::binZ:
      transfoGlobalToLocal = [](const Acts::Vector3& pos) -> double {
        return (pos.z());
      };
      break;

      // case Acts::BinningValue::binRPhi:
      // case Acts::BinningValue::binEta:
      // case Acts::BinningValue::binH:
      // case Acts::BinningValue::binMag:
    default:
      throw std::invalid_argument("Incorrect bin, should be x,y,z,r,phi");
  }
  return transfoGlobalToLocal;
}

Acts::Grid2D Acts::createGrid2D(
    const Acts::BinUtility& bins,
    std::function<Acts::Vector2(Acts::Vector3)>& transfoGlobalToLocal) {
  auto bu = bins.binningData();

  bool isCartesian = false;
  bool isCylindrical = false;

  for (std::size_t b = 0; b < bu.size(); b++) {
    if (bu[b].binvalue == Acts::BinningValue::binX ||
        bu[b].binvalue == Acts::BinningValue::binY) {
      isCartesian = true;
    }
    if (bu[b].binvalue == Acts::BinningValue::binR ||
        bu[b].binvalue == Acts::BinningValue::binPhi) {
      isCylindrical = true;
    }
  }
  if (!(isCartesian || isCylindrical) || (isCylindrical && isCartesian)) {
    throw std::invalid_argument("Incorrect bin, should be x,y,z or r,phi,z");
  }

  // First we need to create the 2 axis
  MaterialGridAxisData gridAxis1{bu[0].min, bu[0].max, bu[0].bins()};
  MaterialGridAxisData gridAxis2{bu[1].min, bu[1].max, bu[1].bins()};

  std::function<double(Acts::Vector3)> coord1 =
      globalToLocalFromBin(bu[0].binvalue);
  std::function<double(Acts::Vector3)> coord2 =
      globalToLocalFromBin(bu[1].binvalue);
  Transform3 transfo = inverseTransform(bins.transform());
  transfoGlobalToLocal = [coord1, coord2,
                          transfo](Acts::Vector3 pos) -> Acts::Vector2 {
    pos = transfo * pos;
    return {coord1(pos), coord2(pos)};
  };
  return Acts::createGrid(gridAxis1, gridAxis2);
}

Acts::Grid3D Acts::createGrid3D(
    const Acts::BinUtility& bins,
    std::function<Acts::Vector3(Acts::Vector3)>& transfoGlobalToLocal) {
  auto bu = bins.binningData();
  // First we need to create the 3 axis

  bool isCartesian = false;
  bool isCylindrical = false;

  for (std::size_t b = 0; b < bu.size(); b++) {
    if (bu[b].binvalue == Acts::BinningValue::binX ||
        bu[b].binvalue == Acts::BinningValue::binY) {
      isCartesian = true;
    }
    if (bu[b].binvalue == Acts::BinningValue::binR ||
        bu[b].binvalue == Acts::BinningValue::binPhi) {
      isCylindrical = true;
    }
  }
  if (!(isCartesian || isCylindrical) || (isCylindrical && isCartesian)) {
    throw std::invalid_argument("Incorrect bin, should be x,y,z or r,phi,z");
  }

  MaterialGridAxisData gridAxis1{bu[0].min, bu[0].max, bu[0].bins()};

  MaterialGridAxisData gridAxis2{bu[1].min, bu[1].max, bu[1].bins()};

  MaterialGridAxisData gridAxis3{bu[2].min, bu[2].max, bu[2].bins()};

  std::function<double(Acts::Vector3)> coord1 =
      globalToLocalFromBin(bu[0].binvalue);
  std::function<double(Acts::Vector3)> coord2 =
      globalToLocalFromBin(bu[1].binvalue);
  std::function<double(Acts::Vector3)> coord3 =
      globalToLocalFromBin(bu[2].binvalue);
  Transform3 transfo = inverseTransform(bins.transform());

  transfoGlobalToLocal = [coord1, coord2, coord3,
                          transfo](Acts::Vector3 pos) -> Acts::Vector3 {
    pos = transfo * pos;
    return {coord1(pos), coord2(pos), coord3(pos)};
  };
  return Acts::createGrid(gridAxis1, gridAxis2, gridAxis3);
}

Acts::MaterialGrid2D Acts::mapMaterialPoints(Acts::Grid2D& grid) {
  // Build material grid
  // Re-build the axes
  Acts::Grid2D::point_t min = grid.minPosition();
  Acts::Grid2D::point_t max = grid.maxPosition();
  Acts::Grid2D::index_t nBins = grid.numLocalBins();

  Acts::EAxis axis1(min[0], max[0], nBins[0]);
  Acts::EAxis axis2(min[1], max[1], nBins[1]);

  // Fill the material Grid by averaging the material in the 2D grid
  Acts::MaterialGrid2D mGrid(std::make_tuple(axis1, axis2));
  for (std::size_t index = 0; index < grid.size(); index++) {
    mGrid.at(index) = grid.at(index).average().parameters();
  }

  return mGrid;
}

Acts::MaterialGrid3D Acts::mapMaterialPoints(Acts::Grid3D& grid) {
  // Build material grid
  // Re-build the axes
  Acts::Grid3D::point_t min = grid.minPosition();
  Acts::Grid3D::point_t max = grid.maxPosition();
  Acts::Grid3D::index_t nBins = grid.numLocalBins();

  Acts::EAxis axis1(min[0], max[0], nBins[0]);
  Acts::EAxis axis2(min[1], max[1], nBins[1]);
  Acts::EAxis axis3(min[2], max[2], nBins[2]);

  // Fill the material Grid by averaging the material in the 3D grid
  Acts::MaterialGrid3D mGrid(std::make_tuple(axis1, axis2, axis3));
  for (std::size_t index = 0; index < grid.size(); index++) {
    mGrid.at(index) = grid.at(index).average().parameters();
  }
  return mGrid;
}
