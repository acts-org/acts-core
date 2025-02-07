// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Detray/DetrayConversionUtils.hpp"

detray::axis::label Acts::DetrayConversionUtils::convertAxisDirection(
    AxisDirection bValue) {
  switch (bValue) {
    case AxisDirection::AxisX:
      return detray::axis::label::e_x;
    case AxisDirection::AxisY:
      return detray::axis::label::e_y;
    case AxisDirection::AxisZ:
      return detray::axis::label::e_z;
    case AxisDirection::AxisR:
      return detray::axis::label::e_r;
    case AxisDirection::AxisPhi:
      return detray::axis::label::e_phi;
    case AxisDirection::AxisRPhi:
      return detray::axis::label::e_rphi;
    default:
      throw std::invalid_argument(
          "DetrayMaterialConverter: unknown binning value detected.");
  }
}

detray::axis::bounds Acts::DetrayConversionUtils::convertBinningOption(
    BinningOption bOption) {
  // That's a bit of a mind bender, but the conversion is correct
  // closed -> axis are closed, i.e. circular
  // open -> axis are not closed, but the range is closed (no overflow bin) ->
  // closed
  switch (bOption) {
    case BinningOption::closed:
      return detray::axis::bounds::e_circular;
    case BinningOption::open:
      return detray::axis::bounds::e_closed;
    default:
      throw std::invalid_argument(
          "DetrayMaterialConverter: unknown binning option detected.");
  }
}

detray::axis::binning Acts::DetrayConversionUtils::convertBinningType(
    BinningType bType) {
  switch (bType) {
    case BinningType::equidistant:
      return detray::axis::binning::e_regular;
    case BinningType::arbitrary:
      return detray::axis::binning::e_irregular;
    default:
      throw std::invalid_argument(
          "DetrayMaterialConverter: unknown binning type detected.");
  }
}

detray::io::axis_payload Acts::DetrayConversionUtils::convertBinningData(
    const BinningData& bData) {
  detray::io::axis_payload axis;

  axis.bins = bData.bins();
  // Set the binning type
  axis.binning = convertBinningType(bData.type);
  // Set the binning option
  axis.bounds = convertBinningOption(bData.option);
  // Set the binning value
  axis.label = convertAxisDirection(bData.binvalue);
  // Set the binning range
  axis.edges = {};
  if (bData.type == BinningType::equidistant) {
    axis.edges = {bData.min, bData.max};
  } else {
    axis.edges.insert(axis.edges.end(), bData.boundaries().begin(),
                      bData.boundaries().end());
  }
  return axis;
}
