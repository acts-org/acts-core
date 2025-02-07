// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/BinUtility.hpp"

#include <numbers>
#include <stdexcept>

namespace Acts {

/// @brief adjust the BinUtility bu to the dimensions of cylinder volume bounds
///
/// @param bu BinUtility at source
/// @param cBounds the Cylinder volume bounds to adjust to
/// @param transform Transform for the adjusted @c BinUtility
///
/// @return new updated BinUtiltiy
BinUtility adjustBinUtility(const BinUtility& bu,
                            const CylinderVolumeBounds& cBounds,
                            const Transform3& transform) {
  // Default constructor
  BinUtility uBinUtil(transform);
  // The parameters from the cylinder bounds
  double minR = cBounds.get(CylinderVolumeBounds::eMinR);
  double maxR = cBounds.get(CylinderVolumeBounds::eMaxR);
  double minPhi = -cBounds.get(CylinderVolumeBounds::eHalfPhiSector);
  double maxPhi = cBounds.get(CylinderVolumeBounds::eHalfPhiSector);
  double minZ = -cBounds.get(CylinderVolumeBounds::eHalfLengthZ);
  double maxZ = cBounds.get(CylinderVolumeBounds::eHalfLengthZ);
  // Retrieve the binning data
  const std::vector<BinningData>& bData = bu.binningData();
  // Loop over the binning data and adjust the dimensions
  for (auto& bd : bData) {
    // The binning value
    AxisDirection bval = bd.binvalue;
    // Throw exceptions is stuff doesn't make sense:
    // - not the right binning value
    // - not equidistant
    if (bd.type == arbitrary) {
      throw std::invalid_argument("Arbitrary binning can not be adjusted.");
    } else if (bval != AxisDirection::AxisR && bval != AxisDirection::AxisPhi &&
               bval != AxisDirection::AxisZ) {
      throw std::invalid_argument("Cylinder volume binning must be: phi, r, z");
    }
    float min = 0;
    float max = 0;
    // Perform the value adjustment
    if (bval == AxisDirection::AxisPhi) {
      min = minPhi;
      max = maxPhi;
    } else if (bval == AxisDirection::AxisR) {
      min = minR;
      max = maxR;
    } else if (bval == AxisDirection::AxisZ) {
      min = minZ;
      max = maxZ;
    }
    // Create the updated BinningData
    BinningData uBinData(bd.option, bval, bd.bins(), min, max);
    uBinUtil += BinUtility(uBinData);
  }

  return uBinUtil;
}

/// @brief adjust the BinUtility bu to the dimensions of cutout cylinder volume
/// bounds
///
/// @param bu BinUtility at source
/// @param cBounds the Cutout Cylinder volume bounds to adjust to
/// @param transform Transform for the adjusted @c BinUtility
///
/// @return new updated BinUtiltiy
BinUtility adjustBinUtility(const BinUtility& bu,
                            const CutoutCylinderVolumeBounds& cBounds,
                            const Transform3& transform) {
  // Default constructor
  BinUtility uBinUtil(transform);
  // The parameters from the cutout cylinder bounds
  double minR = cBounds.get(CutoutCylinderVolumeBounds::eMinR);
  double maxR = cBounds.get(CutoutCylinderVolumeBounds::eMaxR);
  double minPhi = -std::numbers::pi;
  double maxPhi = std::numbers::pi;
  double minZ = -cBounds.get(CutoutCylinderVolumeBounds::eHalfLengthZ);
  double maxZ = cBounds.get(CutoutCylinderVolumeBounds::eHalfLengthZ);
  // Retrieve the binning data
  const std::vector<BinningData>& bData = bu.binningData();
  // Loop over the binning data and adjust the dimensions
  for (auto& bd : bData) {
    // The binning value
    AxisDirection bval = bd.binvalue;
    // Throw exceptions is stuff doesn't make sense:
    // - not the right binning value
    // - not equidistant
    if (bd.type == arbitrary) {
      throw std::invalid_argument("Arbitrary binning can not be adjusted.");
    } else if (bval != AxisDirection::AxisR && bval != AxisDirection::AxisPhi &&
               bval != AxisDirection::AxisZ) {
      throw std::invalid_argument(
          "Cutout cylinder volume binning must be: phi, r, z");
    }
    float min = 0;
    float max = 0;
    // Perform the value adjustment
    if (bval == AxisDirection::AxisPhi) {
      min = minPhi;
      max = maxPhi;
    } else if (bval == AxisDirection::AxisR) {
      min = minR;
      max = maxR;
    } else if (bval == AxisDirection::AxisZ) {
      min = minZ;
      max = maxZ;
    }
    // Create the updated BinningData
    BinningData uBinData(bd.option, bval, bd.bins(), min, max);
    uBinUtil += BinUtility(uBinData);
  }

  return uBinUtil;
}

/// @brief adjust the BinUtility bu to the dimensions of cuboid volume bounds
///
/// @param bu BinUtility at source
/// @param cBounds the Cuboid volume bounds to adjust to
/// @param transform Transform for the adjusted @c BinUtility
///
/// @return new updated BinUtiltiy
BinUtility adjustBinUtility(const BinUtility& bu,
                            const CuboidVolumeBounds& cBounds,
                            const Transform3& transform) {
  // Default constructor
  BinUtility uBinUtil(transform);
  // The parameters from the cylinder bounds
  double minX = -cBounds.get(CuboidVolumeBounds::eHalfLengthX);
  double maxX = cBounds.get(CuboidVolumeBounds::eHalfLengthX);
  double minY = -cBounds.get(CuboidVolumeBounds::eHalfLengthY);
  double maxY = cBounds.get(CuboidVolumeBounds::eHalfLengthY);
  double minZ = -cBounds.get(CuboidVolumeBounds::eHalfLengthZ);
  double maxZ = cBounds.get(CuboidVolumeBounds::eHalfLengthZ);
  // Retrieve the binning data
  const std::vector<BinningData>& bData = bu.binningData();
  // Loop over the binning data and adjust the dimensions
  for (auto& bd : bData) {
    // The binning value
    AxisDirection bval = bd.binvalue;
    // Throw exceptions is stuff doesn't make sense:
    // - not the right binning value
    // - not equidistant
    if (bd.type == arbitrary) {
      throw std::invalid_argument("Arbitrary binning can not be adjusted.");
    } else if (bval != AxisDirection::AxisX && bval != AxisDirection::AxisY &&
               bval != AxisDirection::AxisZ) {
      throw std::invalid_argument("Cylinder volume binning must be: x, y, z");
    }
    float min = 0;
    float max = 0;
    // Perform the value adjustment
    if (bval == AxisDirection::AxisX) {
      min = minX;
      max = maxX;
    } else if (bval == AxisDirection::AxisY) {
      min = minY;
      max = maxY;
    } else if (bval == AxisDirection::AxisZ) {
      min = minZ;
      max = maxZ;
    }
    // Create the updated BinningData
    BinningData uBinData(bd.option, bval, bd.bins(), min, max);
    uBinUtil += BinUtility(uBinData);
  }
  return uBinUtil;
}

/// @brief adjust the BinUtility bu to a volume
///
/// @param bu BinUtility at source
/// @param volume Volume to which the adjustment is being done
///
/// @return new updated BinUtiltiy
BinUtility adjustBinUtility(const BinUtility& bu, const Volume& volume) {
  auto cyBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&(volume.volumeBounds()));
  auto cutcylBounds =
      dynamic_cast<const CutoutCylinderVolumeBounds*>(&(volume.volumeBounds()));
  auto cuBounds =
      dynamic_cast<const CuboidVolumeBounds*>(&(volume.volumeBounds()));

  if (cyBounds != nullptr) {
    // Cylinder bounds
    return adjustBinUtility(bu, *cyBounds, volume.transform());

  } else if (cutcylBounds != nullptr) {
    // Cutout Cylinder bounds
    return adjustBinUtility(bu, *cutcylBounds, volume.transform());

  } else if (cuBounds != nullptr) {
    // Cuboid bounds
    return adjustBinUtility(bu, *cuBounds, volume.transform());
  }

  throw std::invalid_argument(
      "Bin adjustment not implemented for this volume yet!");
}

}  // namespace Acts
