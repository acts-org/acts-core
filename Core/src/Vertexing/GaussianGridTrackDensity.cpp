// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/GaussianGridTrackDensity.hpp"

#include "Acts/Utilities/AlgebraHelpers.hpp"
#include "Acts/Vertexing/VertexingError.hpp"

#include <algorithm>

namespace Acts {

Result<float> GaussianGridTrackDensity::getMaxZPosition(
    MainGridVector& mainGrid) const {
  if (mainGrid.isZero()) {
    return VertexingError::EmptyInput;
  }

  int zbin = -1;
  if (!m_cfg.useHighestSumZPosition) {
    // Get bin with maximum content
    mainGrid.maxCoeff(&zbin);
  } else {
    // Get z position with highest density sum
    // of surrounding bins
    zbin = getHighestSumZPosition(mainGrid);
  }

  // Derive corresponding z value
  return (zbin - m_cfg.mainGridSize / 2.0f + 0.5f) * m_cfg.binSize;
}

Result<std::pair<float, float>>
GaussianGridTrackDensity::getMaxZPositionAndWidth(
    MainGridVector& mainGrid) const {
  // Get z maximum value
  auto maxZRes = getMaxZPosition(mainGrid);
  if (!maxZRes.ok()) {
    return maxZRes.error();
  }
  float maxZ = *maxZRes;

  // Get seed width estimate
  auto widthRes = estimateSeedWidth(mainGrid, maxZ);
  if (!widthRes.ok()) {
    return widthRes.error();
  }
  float width = *widthRes;
  std::pair<float, float> returnPair{maxZ, width};
  return returnPair;
}

std::pair<int, GaussianGridTrackDensity::TrackGridVector>
GaussianGridTrackDensity::addTrack(const BoundTrackParameters& trk,
                                   MainGridVector& mainGrid) const {
  SquareMatrix2 cov = trk.spatialImpactParameterCovariance().value();
  float d0 = trk.parameters()[0];
  float z0 = trk.parameters()[1];

  // Calculate offset in d direction to central bin at z-axis
  int dOffset = static_cast<int>(std::floor(d0 / m_cfg.binSize - 0.5) + 1);
  // Check if current track does affect grid density
  // in central bins at z-axis
  if (std::abs(dOffset) > (m_cfg.trkGridSize - 1) / 2.) {
    // Current track is too far away to contribute
    // to track density at z-axis bins
    return {-1, TrackGridVector::Zero(m_cfg.trkGridSize)};
  }

  // Calculate bin in z
  int zBin = static_cast<int>(z0 / m_cfg.binSize + m_cfg.mainGridSize / 2.);

  if (zBin < 0 || zBin >= m_cfg.mainGridSize) {
    return {-1, TrackGridVector::Zero(m_cfg.trkGridSize)};
  }
  // Calculate the positions of the bin centers
  float binCtrZ = (zBin + 0.5f) * m_cfg.binSize - m_cfg.zMinMax;

  // Calculate the distance between IP values and their
  // corresponding bin centers
  float distCtrZ = z0 - binCtrZ;

  // Create the track grid
  TrackGridVector trackGrid = createTrackGrid(d0, distCtrZ, cov);
  // Add the track grid to the main grid
  addTrackGridToMainGrid(zBin, trackGrid, mainGrid);

  return {zBin, trackGrid};
}

void GaussianGridTrackDensity::addTrackGridToMainGrid(
    int zBin, const TrackGridVector& trkGrid, MainGridVector& mainGrid) const {
  modifyMainGridWithTrackGrid(zBin, trkGrid, mainGrid, +1);
}

void GaussianGridTrackDensity::removeTrackGridFromMainGrid(
    int zBin, const TrackGridVector& trkGrid, MainGridVector& mainGrid) const {
  modifyMainGridWithTrackGrid(zBin, trkGrid, mainGrid, -1);
}

void GaussianGridTrackDensity::modifyMainGridWithTrackGrid(
    int zBin, const TrackGridVector& trkGrid, MainGridVector& mainGrid,
    int modifyModeSign) const {
  int width = (m_cfg.trkGridSize - 1) / 2;
  // Overlap left
  int leftOL = zBin - width;
  // Overlap right
  int rightOL = zBin + width - m_cfg.mainGridSize + 1;
  if (leftOL < 0) {
    int totalTrkSize = m_cfg.trkGridSize + leftOL;
    mainGrid.segment(0, totalTrkSize) +=
        modifyModeSign * trkGrid.segment(-leftOL, totalTrkSize);
    return;
  }
  if (rightOL > 0) {
    int totalTrkSize = m_cfg.trkGridSize - rightOL;
    mainGrid.segment(m_cfg.mainGridSize - totalTrkSize, totalTrkSize) +=
        modifyModeSign * trkGrid.segment(0, totalTrkSize);
    return;
  }

  mainGrid.segment(zBin - width, m_cfg.trkGridSize) += modifyModeSign * trkGrid;
}

GaussianGridTrackDensity::TrackGridVector
GaussianGridTrackDensity::createTrackGrid(float d0, float distCtrZ,
                                          const SquareMatrix2& cov) const {
  TrackGridVector trackGrid(TrackGridVector::Zero(m_cfg.trkGridSize));
  float floorHalfTrkGridSize = static_cast<float>(m_cfg.trkGridSize) / 2 - 0.5f;

  // Loop over columns
  for (int j = 0; j < m_cfg.trkGridSize; j++) {
    float z = (j - floorHalfTrkGridSize) * m_cfg.binSize;
    trackGrid(j) = normal2D(-d0, z - distCtrZ, cov);
  }
  return trackGrid;
}

Result<float> GaussianGridTrackDensity::estimateSeedWidth(
    MainGridVector& mainGrid, float maxZ) const {
  if (mainGrid.isZero()) {
    return VertexingError::EmptyInput;
  }
  // Get z bin of max density z value
  int zBin = static_cast<int>(maxZ / m_cfg.binSize + m_cfg.mainGridSize / 2.);

  const float maxValue = mainGrid(zBin);
  float gridValue = mainGrid(zBin);

  // Find right half-maximum bin
  int rhmBin = zBin;
  while (gridValue > maxValue / 2) {
    rhmBin += 1;
    gridValue = mainGrid(rhmBin);
  }

  // Use linear approximation to find better z value for FWHM between bins
  float deltaZ1 = m_cfg.binSize * (maxValue / 2 - mainGrid(rhmBin - 1)) /
                  (mainGrid(rhmBin) - mainGrid(rhmBin - 1));

  // Find left half-maximum bin
  int lhmBin = zBin;
  gridValue = mainGrid(zBin);
  while (gridValue > maxValue / 2) {
    lhmBin -= 1;
    gridValue = mainGrid(lhmBin);
  }

  // Use linear approximation to find better z value for FWHM between bins
  float deltaZ2 = m_cfg.binSize * (mainGrid(lhmBin + 1) - maxValue / 2) /
                  (mainGrid(lhmBin + 1) - mainGrid(lhmBin));

  // Approximate FWHM
  float fwhm =
      rhmBin * m_cfg.binSize - deltaZ1 - lhmBin * m_cfg.binSize - deltaZ2;

  // FWHM = 2.355 * sigma
  float width = fwhm / 2.355f;

  return std::isnormal(width) ? width : 0.0f;
}

float GaussianGridTrackDensity::normal2D(float d, float z,
                                         const SquareMatrix2& cov) const {
  float det = cov.determinant();
  float coef = 1 / std::sqrt(det);
  float expo =
      -1 / (2 * det) *
      (cov(1, 1) * d * d - (cov(0, 1) + cov(1, 0)) * d * z + cov(0, 0) * z * z);
  return coef * safeExp(expo);
}

int GaussianGridTrackDensity::getHighestSumZPosition(
    MainGridVector& mainGrid) const {
  // Checks the first (up to) 3 density maxima, if they are close, checks which
  // one has the highest surrounding density sum (the two neighboring bins)

  // The global maximum
  int zbin = -1;
  mainGrid.maxCoeff(&zbin);
  int zFirstMax = zbin;
  double firstDensity = mainGrid(zFirstMax);
  double firstSum = getDensitySum(mainGrid, zFirstMax);

  // Get the second highest maximum
  mainGrid[zFirstMax] = 0;
  mainGrid.maxCoeff(&zbin);
  int zSecondMax = zbin;
  double secondDensity = mainGrid(zSecondMax);
  double secondSum = 0;
  if (firstDensity - secondDensity <
      firstDensity * m_cfg.maxRelativeDensityDev) {
    secondSum = getDensitySum(mainGrid, zSecondMax);
  }

  // Get the third highest maximum
  mainGrid[zSecondMax] = 0;
  mainGrid.maxCoeff(&zbin);
  int zThirdMax = zbin;
  double thirdDensity = mainGrid(zThirdMax);
  double thirdSum = 0;
  if (firstDensity - thirdDensity <
      firstDensity * m_cfg.maxRelativeDensityDev) {
    thirdSum = getDensitySum(mainGrid, zThirdMax);
  }

  // Revert back to original values
  mainGrid[zFirstMax] = firstDensity;
  mainGrid[zSecondMax] = secondDensity;

  // Return the z-bin position of the highest density sum
  if (secondSum > firstSum && secondSum > thirdSum) {
    return zSecondMax;
  }
  if (thirdSum > secondSum && thirdSum > firstSum) {
    return zThirdMax;
  }
  return zFirstMax;
}

double GaussianGridTrackDensity::getDensitySum(const MainGridVector& mainGrid,
                                               int pos) const {
  double sum = mainGrid(pos);
  // Sum up only the density contributions from the
  // neighboring bins if they are still within bounds
  if (pos - 1 >= 0) {
    sum += mainGrid(pos - 1);
  }
  if (pos + 1 <= mainGrid.size() - 1) {
    sum += mainGrid(pos + 1);
  }
  return sum;
}

}  // namespace Acts
