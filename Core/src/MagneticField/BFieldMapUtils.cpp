// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/MagneticField/BFieldMapUtils.hpp"

#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/MagneticField/SolenoidBField.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "Acts/Utilities/detail/grid_helper.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include <set>
#include <tuple>

using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;

Acts::InterpolatedBFieldMap<
    Acts::Grid<Acts::Vector2, Acts::Axis<Acts::AxisType::Equidistant>,
               Acts::Axis<Acts::AxisType::Equidistant>>>
Acts::fieldMapRZ(
    const std::function<std::size_t(std::array<std::size_t, 2> binsRZ,
                                    std::array<std::size_t, 2> nBinsRZ)>&
        localToGlobalBin,
    std::vector<double> rPos, std::vector<double> zPos,
    std::vector<Acts::Vector2> bField, double lengthUnit, double BFieldUnit,
    bool firstQuadrant) {
  // [1] Create Grid
  // sort the values
  std::ranges::sort(rPos);
  std::ranges::sort(zPos);
  // Get unique values
  rPos.erase(std::unique(rPos.begin(), rPos.end()), rPos.end());
  zPos.erase(std::unique(zPos.begin(), zPos.end()), zPos.end());
  rPos.shrink_to_fit();
  zPos.shrink_to_fit();
  // get the number of bins
  std::size_t nBinsR = rPos.size();
  std::size_t nBinsZ = zPos.size();

  // get the minimum and maximum. We just sorted the vectors, so these are just
  // the first and last elements.
  double rMin = rPos[0];
  double zMin = zPos[0];
  double rMax = rPos[nBinsR - 1];
  double zMax = zPos[nBinsZ - 1];
  // calculate maxima (add one last bin, because bin value always corresponds to
  // left boundary)
  double stepZ = std::fabs(zMax - zMin) / (nBinsZ - 1);
  double stepR = std::fabs(rMax - rMin) / (nBinsR - 1);
  rMax += stepR;
  zMax += stepZ;
  if (firstQuadrant) {
    zMin = -zPos[nBinsZ - 1];
    nBinsZ = static_cast<std::size_t>(2. * nBinsZ - 1);
  }

  // Create the axis for the grid
  Acts::Axis rAxis(rMin * lengthUnit, rMax * lengthUnit, nBinsR);
  Acts::Axis zAxis(zMin * lengthUnit, zMax * lengthUnit, nBinsZ);

  // Create the grid
  Grid grid(Type<Acts::Vector2>, std::move(rAxis), std::move(zAxis));
  using Grid_t = decltype(grid);

  // [2] Set the bField values
  for (std::size_t i = 1; i <= nBinsR; ++i) {
    for (std::size_t j = 1; j <= nBinsZ; ++j) {
      std::array<std::size_t, 2> nIndices = {{rPos.size(), zPos.size()}};
      Grid_t::index_t indices = {{i, j}};
      if (firstQuadrant) {
        // std::vectors begin with 0 and we do not want the user needing to
        // take underflow or overflow bins in account this is why we need to
        // subtract by one
        std::size_t n =
            std::abs(static_cast<int>(j) - static_cast<int>(zPos.size()));
        Grid_t::index_t indicesFirstQuadrant = {{i - 1, n}};

        grid.atLocalBins(indices) =
            bField.at(localToGlobalBin(indicesFirstQuadrant, nIndices)) *
            BFieldUnit;
      } else {
        // std::vectors begin with 0 and we do not want the user needing to
        // take underflow or overflow bins in account this is why we need to
        // subtract by one
        grid.atLocalBins(indices) =
            bField.at(localToGlobalBin({{i - 1, j - 1}}, nIndices)) *
            BFieldUnit;
      }
    }
  }
  grid.setExteriorBins(Acts::Vector2::Zero());

  // [3] Create the transformation for the position
  // map (x,y,z) -> (r,z)
  auto transformPos = [](const Acts::Vector3& pos) {
    return Acts::Vector2(perp(pos), pos.z());
  };

  // [4] Create the transformation for the bfield
  // map (Br,Bz) -> (Bx,By,Bz)
  auto transformBField = [](const Acts::Vector2& field,
                            const Acts::Vector3& pos) {
    double r_sin_theta_2 = pos.x() * pos.x() + pos.y() * pos.y();
    double cos_phi = 0, sin_phi = 0;
    if (r_sin_theta_2 > std::numeric_limits<double>::min()) {
      double inv_r_sin_theta = 1. / sqrt(r_sin_theta_2);
      cos_phi = pos.x() * inv_r_sin_theta;
      sin_phi = pos.y() * inv_r_sin_theta;
    } else {
      cos_phi = 1.;
      sin_phi = 0.;
    }
    return Acts::Vector3(field.x() * cos_phi, field.x() * sin_phi, field.y());
  };

  // [5] Create the mapper & BField Service
  // create field mapping
  return Acts::InterpolatedBFieldMap<Grid_t>(
      {transformPos, transformBField, std::move(grid)});
}

Acts::InterpolatedBFieldMap<
    Acts::Grid<Acts::Vector3, Acts::Axis<Acts::AxisType::Equidistant>,
               Acts::Axis<Acts::AxisType::Equidistant>,
               Acts::Axis<Acts::AxisType::Equidistant>>>
Acts::fieldMapXYZ(
    const std::function<std::size_t(std::array<std::size_t, 3> binsXYZ,
                                    std::array<std::size_t, 3> nBinsXYZ)>&
        localToGlobalBin,
    std::vector<double> xPos, std::vector<double> yPos,
    std::vector<double> zPos, std::vector<Acts::Vector3> bField,
    double lengthUnit, double BFieldUnit, bool firstOctant) {
  // [1] Create Grid
  // Sort the values
  std::ranges::sort(xPos);
  std::ranges::sort(yPos);
  std::ranges::sort(zPos);
  // Get unique values
  xPos.erase(std::unique(xPos.begin(), xPos.end()), xPos.end());
  yPos.erase(std::unique(yPos.begin(), yPos.end()), yPos.end());
  zPos.erase(std::unique(zPos.begin(), zPos.end()), zPos.end());
  xPos.shrink_to_fit();
  yPos.shrink_to_fit();
  zPos.shrink_to_fit();
  // get the number of bins
  std::size_t nBinsX = xPos.size();
  std::size_t nBinsY = yPos.size();
  std::size_t nBinsZ = zPos.size();

  // Create the axis for the grid
  // get minima and maximia. We just sorted the vectors, so these are just the
  // first and last elements.
  double xMin = xPos[0];
  double yMin = yPos[0];
  double zMin = zPos[0];
  // get maxima
  double xMax = xPos[nBinsX - 1];
  double yMax = yPos[nBinsY - 1];
  double zMax = zPos[nBinsZ - 1];
  // calculate maxima (add one last bin, because bin value always corresponds to
  // left boundary)
  double stepZ = std::fabs(zMax - zMin) / (nBinsZ - 1);
  double stepY = std::fabs(yMax - yMin) / (nBinsY - 1);
  double stepX = std::fabs(xMax - xMin) / (nBinsX - 1);
  xMax += stepX;
  yMax += stepY;
  zMax += stepZ;

  // If only the first octant is given
  if (firstOctant) {
    xMin = -xPos[nBinsX - 1];
    yMin = -yPos[nBinsY - 1];
    zMin = -zPos[nBinsZ - 1];
    nBinsX = 2 * nBinsX - 1;
    nBinsY = 2 * nBinsY - 1;
    nBinsZ = 2 * nBinsZ - 1;
  }
  Acts::Axis xAxis(xMin * lengthUnit, xMax * lengthUnit, nBinsX);
  Acts::Axis yAxis(yMin * lengthUnit, yMax * lengthUnit, nBinsY);
  Acts::Axis zAxis(zMin * lengthUnit, zMax * lengthUnit, nBinsZ);
  // Create the grid
  Grid grid(Type<Vector3>, std::move(xAxis), std::move(yAxis),
            std::move(zAxis));
  using Grid_t = decltype(grid);

  // [2] Set the bField values
  for (std::size_t i = 1; i <= nBinsX; ++i) {
    for (std::size_t j = 1; j <= nBinsY; ++j) {
      for (std::size_t k = 1; k <= nBinsZ; ++k) {
        Grid_t::index_t indices = {{i, j, k}};
        std::array<std::size_t, 3> nIndices = {
            {xPos.size(), yPos.size(), zPos.size()}};
        if (firstOctant) {
          // std::vectors begin with 0 and we do not want the user needing to
          // take underflow or overflow bins in account this is why we need to
          // subtract by one
          std::size_t m =
              std::abs(static_cast<int>(i) - (static_cast<int>(xPos.size())));
          std::size_t n =
              std::abs(static_cast<int>(j) - (static_cast<int>(yPos.size())));
          std::size_t l =
              std::abs(static_cast<int>(k) - (static_cast<int>(zPos.size())));
          Grid_t::index_t indicesFirstOctant = {{m, n, l}};

          grid.atLocalBins(indices) =
              bField.at(localToGlobalBin(indicesFirstOctant, nIndices)) *
              BFieldUnit;

        } else {
          // std::vectors begin with 0 and we do not want the user needing to
          // take underflow or overflow bins in account this is why we need to
          // subtract by one
          grid.atLocalBins(indices) =
              bField.at(localToGlobalBin({{i - 1, j - 1, k - 1}}, nIndices)) *
              BFieldUnit;
        }
      }
    }
  }
  grid.setExteriorBins(Acts::Vector3::Zero());

  // [3] Create the transformation for the position
  // map (x,y,z) -> (r,z)
  auto transformPos = [](const Acts::Vector3& pos) { return pos; };

  // [4] Create the transformation for the bfield
  // map (Bx,By,Bz) -> (Bx,By,Bz)
  auto transformBField = [](const Acts::Vector3& field,
                            const Acts::Vector3& /*pos*/) { return field; };

  // [5] Create the mapper & BField Service
  // create field mapping
  return Acts::InterpolatedBFieldMap<Grid_t>(
      {transformPos, transformBField, std::move(grid)});
}

Acts::InterpolatedBFieldMap<
    Acts::Grid<Acts::Vector2, Acts::Axis<Acts::AxisType::Equidistant>,
               Acts::Axis<Acts::AxisType::Equidistant>>>
Acts::solenoidFieldMap(std::pair<double, double> rlim,
                       std::pair<double, double> zlim,
                       std::pair<std::size_t, std::size_t> nbins,
                       const SolenoidBField& field) {
  double rMin = 0, rMax = 0, zMin = 0, zMax = 0;
  std::tie(rMin, rMax) = rlim;
  std::tie(zMin, zMax) = zlim;

  std::size_t nBinsR = 0, nBinsZ = 0;
  std::tie(nBinsR, nBinsZ) = nbins;

  double stepZ = std::abs(zMax - zMin) / (nBinsZ - 1);
  double stepR = std::abs(rMax - rMin) / (nBinsR - 1);

  rMax += stepR;
  zMax += stepZ;

  // Create the axis for the grid
  Acts::Axis rAxis(rMin, rMax, nBinsR);
  Acts::Axis zAxis(zMin, zMax, nBinsZ);

  // Create the grid
  Grid grid(Type<Acts::Vector2>, std::move(rAxis), std::move(zAxis));
  using Grid_t = decltype(grid);

  // Create the transformation for the position
  // map (x,y,z) -> (r,z)
  auto transformPos = [](const Acts::Vector3& pos) {
    return Acts::Vector2(perp(pos), pos.z());
  };

  // Create the transformation for the bfield
  // map (Br,Bz) -> (Bx,By,Bz)
  auto transformBField = [](const Acts::Vector2& bfield,
                            const Acts::Vector3& pos) {
    double r_sin_theta_2 = pos.x() * pos.x() + pos.y() * pos.y();
    double cos_phi = 0, sin_phi = 0;
    if (r_sin_theta_2 > std::numeric_limits<double>::min()) {
      double inv_r_sin_theta = 1. / sqrt(r_sin_theta_2);
      cos_phi = pos.x() * inv_r_sin_theta;
      sin_phi = pos.y() * inv_r_sin_theta;
    } else {
      cos_phi = 1.;
      sin_phi = 0.;
    }
    return Acts::Vector3(bfield.x() * cos_phi, bfield.x() * sin_phi,
                         bfield.y());
  };

  // iterate over all bins, set their value to the solenoid value
  // at their lower left position
  for (std::size_t i = 0; i <= nBinsR + 1; i++) {
    for (std::size_t j = 0; j <= nBinsZ + 1; j++) {
      Grid_t::index_t index({i, j});
      if (i == 0 || j == 0 || i == nBinsR + 1 || j == nBinsZ + 1) {
        // under or overflow bin, set zero
        grid.atLocalBins(index) = Grid_t::value_type(0, 0);
      } else {
        // regular bin, get lower left boundary
        Grid_t::point_t lowerLeft = grid.lowerLeftBinEdge(index);
        // do lookup
        Vector2 B = field.getField(Vector2(lowerLeft[0], lowerLeft[1]));
        grid.atLocalBins(index) = B;
      }
    }
  }

  // Create the mapper & BField Service
  // create field mapping
  Acts::InterpolatedBFieldMap<Grid_t> map(
      {transformPos, transformBField, std::move(grid)});
  return map;
}
