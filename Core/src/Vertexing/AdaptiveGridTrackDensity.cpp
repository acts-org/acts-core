// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/AdaptiveGridTrackDensity.hpp"

#include "Acts/Utilities/AlgebraHelpers.hpp"
#include "Acts/Vertexing/VertexingError.hpp"

#include <algorithm>

namespace Acts {

namespace {

/// @brief Helper to retrieve values of an nDim-dimensional normal
/// distribution
/// @note The constant prefactor (2 * pi)^(- nDim / 2) is discarded
///
/// @param args Coordinates where the Gaussian should be evaluated
/// @note args must be in a coordinate system with origin at the mean
/// values of the Gaussian
/// @param cov Covariance matrix
///
/// @return Multivariate Gaussian evaluated at args
template <unsigned int nDim>
double multivariateGaussian(const ActsVector<nDim>& args,
                            const ActsSquareMatrix<nDim>& cov) {
  double exponent = -0.5 * args.transpose().dot(cov.inverse() * args);
  double gaussianDensity = safeExp(exponent) / std::sqrt(cov.determinant());
  return gaussianDensity;
}

}  // namespace

double AdaptiveGridTrackDensity::getBinCenter(std::int32_t bin,
                                              double binExtent) {
  return bin * binExtent;
}

std::int32_t AdaptiveGridTrackDensity::getBin(double value, double binExtent) {
  return static_cast<int>(std::floor(value / binExtent - 0.5) + 1);
}

std::uint32_t AdaptiveGridTrackDensity::getTrkGridSize(
    double sigma, double nTrkSigmas, double binExtent,
    const GridSizeRange& trkGridSizeRange) {
  std::uint32_t size =
      static_cast<std::uint32_t>(std::ceil(2 * nTrkSigmas * sigma / binExtent));
  // Make sure the grid size is odd
  if (size % 2 == 0) {
    ++size;
  }
  // Make sure the grid size is within the allowed range
  if (trkGridSizeRange.first && size < trkGridSizeRange.first.value()) {
    size = trkGridSizeRange.first.value();
  }
  if (trkGridSizeRange.second && size > trkGridSizeRange.second.value()) {
    size = trkGridSizeRange.second.value();
  }
  return size;
}

std::int32_t AdaptiveGridTrackDensity::getSpatialBin(double value) const {
  return getBin(value, m_cfg.spatialBinExtent);
}

std::int32_t AdaptiveGridTrackDensity::getTemporalBin(double value) const {
  if (!m_cfg.useTime) {
    return 0;
  }
  return getBin(value, m_cfg.temporalBinExtent);
}

double AdaptiveGridTrackDensity::getSpatialBinCenter(std::int32_t bin) const {
  return getBinCenter(bin, m_cfg.spatialBinExtent);
}

double AdaptiveGridTrackDensity::getTemporalBinCenter(std::int32_t bin) const {
  if (!m_cfg.useTime) {
    return 0.;
  }
  return getBinCenter(bin, m_cfg.temporalBinExtent);
}

std::uint32_t AdaptiveGridTrackDensity::getSpatialTrkGridSize(
    double sigma) const {
  return getTrkGridSize(sigma, m_cfg.nSpatialTrkSigmas, m_cfg.spatialBinExtent,
                        m_cfg.spatialTrkGridSizeRange);
}

std::uint32_t AdaptiveGridTrackDensity::getTemporalTrkGridSize(
    double sigma) const {
  if (!m_cfg.useTime) {
    return 1;
  }
  return getTrkGridSize(sigma, m_cfg.nTemporalTrkSigmas,
                        m_cfg.temporalBinExtent,
                        m_cfg.temporalTrkGridSizeRange);
}

AdaptiveGridTrackDensity::AdaptiveGridTrackDensity(const Config& cfg)
    : m_cfg(cfg) {
  if (m_cfg.spatialTrkGridSizeRange.first &&
      m_cfg.spatialTrkGridSizeRange.first.value() % 2 == 0) {
    throw std::invalid_argument(
        "AdaptiveGridTrackDensity: spatialTrkGridSizeRange.first must be odd");
  }
  if (m_cfg.spatialTrkGridSizeRange.second &&
      m_cfg.spatialTrkGridSizeRange.second.value() % 2 == 0) {
    throw std::invalid_argument(
        "AdaptiveGridTrackDensity: spatialTrkGridSizeRange.second must be odd");
  }
  if (m_cfg.temporalTrkGridSizeRange.first &&
      m_cfg.temporalTrkGridSizeRange.first.value() % 2 == 0) {
    throw std::invalid_argument(
        "AdaptiveGridTrackDensity: temporalTrkGridSizeRange.first must be odd");
  }
  if (m_cfg.temporalTrkGridSizeRange.second &&
      m_cfg.temporalTrkGridSizeRange.second.value() % 2 == 0) {
    throw std::invalid_argument(
        "AdaptiveGridTrackDensity: temporalTrkGridSizeRange.second must be "
        "odd");
  }
}

AdaptiveGridTrackDensity::DensityMap::const_iterator
AdaptiveGridTrackDensity::highestDensityEntry(
    const DensityMap& densityMap) const {
  auto maxEntry = std::max_element(
      std::begin(densityMap), std::end(densityMap),
      [](const auto& a, const auto& b) { return a.second < b.second; });
  return maxEntry;
}

Result<AdaptiveGridTrackDensity::ZTPosition>
AdaptiveGridTrackDensity::getMaxZTPosition(DensityMap& densityMap) const {
  if (densityMap.empty()) {
    return VertexingError::EmptyInput;
  }

  Bin bin;
  if (!m_cfg.useHighestSumZPosition) {
    bin = highestDensityEntry(densityMap)->first;
  } else {
    // Get z position with highest density sum
    // of surrounding bins
    bin = highestDensitySumBin(densityMap);
  }

  // Derive corresponding z and t value
  double maxZ = getSpatialBinCenter(bin.first);
  double maxT = getTemporalBinCenter(bin.second);

  return std::make_pair(maxZ, maxT);
}

Result<AdaptiveGridTrackDensity::ZTPositionAndWidth>
AdaptiveGridTrackDensity::getMaxZTPositionAndWidth(
    DensityMap& densityMap) const {
  // Get z value where the density is the highest
  auto maxZTRes = getMaxZTPosition(densityMap);
  if (!maxZTRes.ok()) {
    return maxZTRes.error();
  }
  ZTPosition maxZT = *maxZTRes;

  // Get seed width estimate
  auto widthRes = estimateSeedWidth(densityMap, maxZT);
  if (!widthRes.ok()) {
    return widthRes.error();
  }
  double width = *widthRes;
  ZTPositionAndWidth maxZTAndWidth{maxZT, width};
  return maxZTAndWidth;
}

AdaptiveGridTrackDensity::DensityMap AdaptiveGridTrackDensity::addTrack(
    const BoundTrackParameters& trk, DensityMap& mainDensityMap) const {
  ActsVector<3> impactParams = trk.impactParameters();
  ActsSquareMatrix<3> cov = trk.impactParameterCovariance().value();

  std::uint32_t spatialTrkGridSize =
      getSpatialTrkGridSize(std::sqrt(cov(1, 1)));
  std::uint32_t temporalTrkGridSize =
      getTemporalTrkGridSize(std::sqrt(cov(2, 2)));

  // Calculate bin in d direction
  std::int32_t centralDBin = getBin(impactParams(0), m_cfg.spatialBinExtent);
  // Check if current track affects grid density
  if (std::abs(centralDBin) > (spatialTrkGridSize - 1) / 2.) {
    // Return empty map
    return {};
  }

  // Calculate bin in z and t direction
  std::int32_t centralZBin = getSpatialBin(impactParams(1));
  std::int32_t centralTBin = getTemporalBin(impactParams(2));

  Bin centralBin = {centralZBin, centralTBin};

  DensityMap trackDensityMap = createTrackGrid(
      impactParams, centralBin, cov, spatialTrkGridSize, temporalTrkGridSize);

  for (const auto& [bin, density] : trackDensityMap) {
    mainDensityMap[bin] += density;
  }

  return trackDensityMap;
}

void AdaptiveGridTrackDensity::subtractTrack(const DensityMap& trackDensityMap,
                                             DensityMap& mainDensityMap) const {
  for (const auto& [bin, density] : trackDensityMap) {
    mainDensityMap[bin] -= density;
  }
}

AdaptiveGridTrackDensity::DensityMap AdaptiveGridTrackDensity::createTrackGrid(
    const Vector3& impactParams, const Bin& centralBin,
    const SquareMatrix3& cov, std::uint32_t spatialTrkGridSize,
    std::uint32_t temporalTrkGridSize) const {
  DensityMap trackDensityMap;

  std::uint32_t halfSpatialTrkGridSize = (spatialTrkGridSize - 1) / 2;
  std::int32_t firstZBin = centralBin.first - halfSpatialTrkGridSize;

  // If we don't do time vertex seeding, firstTBin will be 0.
  std::uint32_t halfTemporalTrkGridSize = (temporalTrkGridSize - 1) / 2;
  std::int32_t firstTBin = centralBin.second - halfTemporalTrkGridSize;

  // Loop over bins
  for (std::uint32_t i = 0; i < temporalTrkGridSize; i++) {
    std::int32_t tBin = firstTBin + i;
    double t = getTemporalBinCenter(tBin);
    if (t < m_cfg.temporalWindow.first || t > m_cfg.temporalWindow.second) {
      continue;
    }
    for (std::uint32_t j = 0; j < spatialTrkGridSize; j++) {
      std::int32_t zBin = firstZBin + j;
      double z = getSpatialBinCenter(zBin);
      if (z < m_cfg.spatialWindow.first || z > m_cfg.spatialWindow.second) {
        continue;
      }
      // Bin coordinates in the d-z-t plane
      Vector3 binCoords(0., z, t);
      // Transformation to coordinate system with origin at the track center
      binCoords -= impactParams;
      Bin bin = {zBin, tBin};
      double density = 0;
      if (m_cfg.useTime) {
        density = multivariateGaussian<3>(binCoords, cov);
      } else {
        density = multivariateGaussian<2>(binCoords.head<2>(),
                                          cov.topLeftCorner<2, 2>());
      }
      // Only add density if it is positive (otherwise it is 0)
      if (density > 0) {
        trackDensityMap[bin] = density;
      }
    }
  }

  return trackDensityMap;
}

Result<double> AdaptiveGridTrackDensity::estimateSeedWidth(
    const DensityMap& densityMap, const ZTPosition& maxZT) const {
  if (densityMap.empty()) {
    return VertexingError::EmptyInput;
  }

  // Get z and t bin of max density
  std::int32_t zMaxBin = getBin(maxZT.first, m_cfg.spatialBinExtent);
  std::int32_t tMaxBin = getBin(maxZT.second, m_cfg.temporalBinExtent);
  double maxValue = densityMap.at({zMaxBin, tMaxBin});

  std::int32_t rhmBin = zMaxBin;
  double gridValue = maxValue;
  // Boolean indicating whether we find a filled bin that has a densityValue <=
  // maxValue/2
  bool binFilled = true;
  while (gridValue > maxValue / 2) {
    // Check if we are still operating on continuous z values
    if (!densityMap.contains({rhmBin + 1, tMaxBin})) {
      binFilled = false;
      break;
    }
    rhmBin += 1;
    gridValue = densityMap.at({rhmBin, tMaxBin});
  }

  // Use linear approximation to find better z value for FWHM between bins
  double rightDensity = 0;
  if (binFilled) {
    rightDensity = densityMap.at({rhmBin, tMaxBin});
  }
  double leftDensity = densityMap.at({rhmBin - 1, tMaxBin});
  double deltaZ1 = m_cfg.spatialBinExtent * (maxValue / 2 - leftDensity) /
                   (rightDensity - leftDensity);

  std::int32_t lhmBin = zMaxBin;
  gridValue = maxValue;
  binFilled = true;
  while (gridValue > maxValue / 2) {
    // Check if we are still operating on continuous z values
    if (!densityMap.contains({lhmBin - 1, tMaxBin})) {
      binFilled = false;
      break;
    }
    lhmBin -= 1;
    gridValue = densityMap.at({lhmBin, tMaxBin});
  }

  // Use linear approximation to find better z value for FWHM between bins
  rightDensity = densityMap.at({lhmBin + 1, tMaxBin});
  if (binFilled) {
    leftDensity = densityMap.at({lhmBin, tMaxBin});
  } else {
    leftDensity = 0;
  }
  double deltaZ2 = m_cfg.spatialBinExtent * (rightDensity - maxValue / 2) /
                   (rightDensity - leftDensity);

  double fwhm = (rhmBin - lhmBin) * m_cfg.spatialBinExtent - deltaZ1 - deltaZ2;

  // FWHM = 2.355 * sigma
  double width = fwhm / 2.355;

  return std::isnormal(width) ? width : 0.0;
}

AdaptiveGridTrackDensity::Bin AdaptiveGridTrackDensity::highestDensitySumBin(
    DensityMap& densityMap) const {
  // The global maximum
  auto firstMax = highestDensityEntry(densityMap);
  Bin binFirstMax = firstMax->first;
  double valueFirstMax = firstMax->second;
  double firstSum = getDensitySum(densityMap, binFirstMax);
  // Smaller maxima must have a density of at least:
  // valueFirstMax - densityDeviation
  double densityDeviation = valueFirstMax * m_cfg.maxRelativeDensityDev;

  // Get the second highest maximum
  densityMap[binFirstMax] = 0;
  auto secondMax = highestDensityEntry(densityMap);
  Bin binSecondMax = secondMax->first;
  double valueSecondMax = secondMax->second;
  double secondSum = 0;
  if (valueFirstMax - valueSecondMax < densityDeviation) {
    secondSum = getDensitySum(densityMap, binSecondMax);
  } else {
    // If the second maximum is not sufficiently large the third maximum won't
    // be either
    densityMap[binFirstMax] = valueFirstMax;
    return binFirstMax;
  }

  // Get the third highest maximum
  densityMap[binSecondMax] = 0;
  auto thirdMax = highestDensityEntry(densityMap);
  Bin binThirdMax = thirdMax->first;
  double valueThirdMax = thirdMax->second;
  double thirdSum = 0;
  if (valueFirstMax - valueThirdMax < densityDeviation) {
    thirdSum = getDensitySum(densityMap, binThirdMax);
  }

  // Revert back to original values
  densityMap[binFirstMax] = valueFirstMax;
  densityMap[binSecondMax] = valueSecondMax;

  // Return the z bin position of the highest density sum
  if (secondSum > firstSum && secondSum > thirdSum) {
    return binSecondMax;
  }
  if (thirdSum > secondSum && thirdSum > firstSum) {
    return binThirdMax;
  }
  return binFirstMax;
}

double AdaptiveGridTrackDensity::getDensitySum(const DensityMap& densityMap,
                                               const Bin& bin) const {
  auto valueOrZero = [&densityMap](const Bin& b) {
    if (auto it = densityMap.find(b); it != densityMap.end()) {
      return it->second;
    }
    return 0.0f;
  };

  // Add density from the bin.
  // Check if neighboring bins are part of the densityMap and add them (if they
  // are not part of the map, we assume them to be 0).
  return valueOrZero(bin) + valueOrZero({bin.first - 1, bin.second}) +
         valueOrZero({bin.first + 1, bin.second});
}

}  // namespace Acts
