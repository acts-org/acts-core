// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/MagneticField/BFieldMapUtils.hpp"

#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/MagneticField/SolenoidBField.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "Acts/Utilities/detail/grid_helper.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
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
    const std::vector<Vector2>& bField, double lengthUnit, double BFieldUnit,
    bool firstQuadrant) {
  // [1] Create Grid
  const auto [rMin, rMax, rBinCount] = detail::getMinMaxAndBinCount(rPos);
  auto [zMin, zMax, zBinCount] = detail::getMinMaxAndBinCount(zPos);

  const std::size_t nBinsR = rBinCount;
  std::size_t nBinsZ = zBinCount;

  if (firstQuadrant) {
    zMin = -zPos[nBinsZ - 1];
    nBinsZ = 2 * nBinsZ - 1;
  }

  // Create the axis for the grid
  Axis rAxis(rMin * lengthUnit, rMax * lengthUnit, nBinsR);
  Axis zAxis(zMin * lengthUnit, zMax * lengthUnit, nBinsZ);

  // Create the grid
  Grid grid(Type<Vector2>, std::move(rAxis), std::move(zAxis));
  using Grid_t = decltype(grid);

  // [2] Set the bField values
  const std::array<std::size_t, 2> nIndices = {{rBinCount, zBinCount}};
  for (std::size_t i = 1; i <= nBinsR; ++i) {
    for (std::size_t j = 1; j <= nBinsZ; ++j) {
      Grid_t::index_t indices = {{i, j}};
      // std::vectors begin with 0 and we do not want the user needing to take
      // underflow or overflow bins in account this is why we need to subtract
      // by one
      if (firstQuadrant) {
        std::size_t n = std::abs(static_cast<std::ptrdiff_t>(j) -
                                 static_cast<std::ptrdiff_t>(zBinCount));

        grid.atLocalBins(indices) =
            bField.at(localToGlobalBin({{i - 1, n}}, nIndices)) * BFieldUnit;
      } else {
        grid.atLocalBins(indices) =
            bField.at(localToGlobalBin({{i - 1, j - 1}}, nIndices)) *
            BFieldUnit;
      }
    }
  }
  grid.setExteriorBins(Vector2::Zero());

  // [3] Create the transformation for the position map (x,y,z) -> (r,z)
  auto transformPos = [](const Vector3& pos) {
    return Vector2(perp(pos), pos.z());
  };

  // [4] Create the transformation for the bField map (Br,Bz) -> (Bx,By,Bz)
  auto transformBField = [](const Vector2& field, const Vector3& pos) {
    const double rSinTheta2 = pos.x() * pos.x() + pos.y() * pos.y();
    double cosPhi = 1.;
    double sinPhi = 0.;

    if (rSinTheta2 > std::numeric_limits<double>::min()) {
      const double invRsinTheta = 1. / std::sqrt(rSinTheta2);
      cosPhi = pos.x() * invRsinTheta;
      sinPhi = pos.y() * invRsinTheta;
    }

    return Vector3(field.x() * cosPhi, field.x() * sinPhi, field.y());
  };

  // [5] Create the mapper & BField Service create field mapping
  return InterpolatedBFieldMap<Grid_t>(
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
    std::vector<double> zPos, const std::vector<Vector3>& bField,
    double lengthUnit, double BFieldUnit, bool firstOctant) {
  // [1] Create Grid
  auto [xMin, xMax, xBinCount] = detail::getMinMaxAndBinCount(xPos);
  auto [yMin, yMax, yBinCount] = detail::getMinMaxAndBinCount(yPos);
  auto [zMin, zMax, zBinCount] = detail::getMinMaxAndBinCount(zPos);

  std::size_t nBinsX = xBinCount;
  std::size_t nBinsY = yBinCount;
  std::size_t nBinsZ = zBinCount;

  if (firstOctant) {
    xMin = -xPos[nBinsX - 1];
    nBinsX = 2 * nBinsX - 1;
    yMin = -yPos[nBinsY - 1];
    nBinsY = 2 * nBinsY - 1;
    zMin = -zPos[nBinsZ - 1];
    nBinsZ = 2 * nBinsZ - 1;
  }

  Axis xAxis(xMin * lengthUnit, xMax * lengthUnit, nBinsX);
  Axis yAxis(yMin * lengthUnit, yMax * lengthUnit, nBinsY);
  Axis zAxis(zMin * lengthUnit, zMax * lengthUnit, nBinsZ);
  // Create the grid
  Grid grid(Type<Vector3>, std::move(xAxis), std::move(yAxis),
            std::move(zAxis));
  using Grid_t = decltype(grid);

  // [2] Set the bField values
  const std::array<std::size_t, 3> nIndices = {
      {xBinCount, yBinCount, zBinCount}};

  auto calcAbsDiff = [](std::size_t val, std::size_t binCount) {
    return std::abs(static_cast<std::ptrdiff_t>(val) -
                    static_cast<std::ptrdiff_t>(binCount));
  };

  for (std::size_t i = 1; i <= nBinsX; ++i) {
    for (std::size_t j = 1; j <= nBinsY; ++j) {
      for (std::size_t k = 1; k <= nBinsZ; ++k) {
        Grid_t::index_t indices = {{i, j, k}};
        // std::vectors begin with 0 and we do not want the user needing to take
        // underflow or overflow bins in account this is why we need to subtract
        // by one
        if (firstOctant) {
          const std::size_t l = calcAbsDiff(i, xBinCount);
          const std::size_t m = calcAbsDiff(j, yBinCount);
          const std::size_t n = calcAbsDiff(k, zBinCount);

          grid.atLocalBins(indices) =
              bField.at(localToGlobalBin({{l, m, n}}, nIndices)) * BFieldUnit;
        } else {
          grid.atLocalBins(indices) =
              bField.at(localToGlobalBin({{i - 1, j - 1, k - 1}}, nIndices)) *
              BFieldUnit;
        }
      }
    }
  }
  grid.setExteriorBins(Vector3::Zero());

  // [3] Create the transformation for the position map (x,y,z) -> (r,z)
  auto transformPos = [](const Vector3& pos) { return pos; };

  // [4] Create the transformation for the BField map (Bx,By,Bz) -> (Bx,By,Bz)
  auto transformBField = [](const Vector3& field, const Vector3& /*pos*/) {
    return field;
  };

  // [5] Create the mapper & BField Service create field mapping
  return InterpolatedBFieldMap<Grid_t>(
      {transformPos, transformBField, std::move(grid)});
}

Acts::InterpolatedBFieldMap<
    Acts::Grid<Acts::Vector2, Acts::Axis<Acts::AxisType::Equidistant>,
               Acts::Axis<Acts::AxisType::Equidistant>>>
Acts::solenoidFieldMap(const std::pair<double, double>& rLim,
                       const std::pair<double, double>& zLim,
                       const std::pair<std::size_t, std::size_t>& nBins,
                       const SolenoidBField& field) {
  auto [rMin, rMax] = rLim;
  auto [zMin, zMax] = zLim;
  const auto [nBinsR, nBinsZ] = nBins;

  double stepZ = std::abs(zMax - zMin) / (nBinsZ - 1);
  double stepR = std::abs(rMax - rMin) / (nBinsR - 1);
  rMax += stepR;
  zMax += stepZ;

  // Create the axis for the grid
  Axis rAxis(rMin, rMax, nBinsR);
  Axis zAxis(zMin, zMax, nBinsZ);

  // Create the grid
  Grid grid(Type<Vector2>, std::move(rAxis), std::move(zAxis));
  using Grid_t = decltype(grid);

  // Create the transformation for the position map (x,y,z) -> (r,z)
  auto transformPos = [](const Vector3& pos) {
    return Vector2(perp(pos), pos.z());
  };

  // Create the transformation for the bField map (Br,Bz) -> (Bx,By,Bz)
  auto transformBField = [](const Vector2& bField, const Vector3& pos) {
    const double rSinTheta2 = pos.x() * pos.x() + pos.y() * pos.y();
    double cosPhi = 1.;
    double sinPhi = 0.;

    if (rSinTheta2 > std::numeric_limits<double>::min()) {
      const double invRsinTheta = 1. / std::sqrt(rSinTheta2);
      cosPhi = pos.x() * invRsinTheta;
      sinPhi = pos.y() * invRsinTheta;
    }

    return Vector3(bField.x() * cosPhi, bField.x() * sinPhi, bField.y());
  };

  // iterate over all bins, set their value to the solenoid value at their lower
  // left position
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

  // Create the mapper & BField Service create field mapping
  InterpolatedBFieldMap<Grid_t> map(
      {transformPos, transformBField, std::move(grid)});
  return map;
}
