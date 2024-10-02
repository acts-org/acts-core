// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Detray/DetrayConversionUtils.hpp"

detray::axis::label Acts::DetrayConversionUtils::convertBinningValue(
    BinningValue bValue) {
  switch (bValue) {
    case BinningValue::binX:
      return detray::axis::label::e_x;
    case BinningValue::binY:
      return detray::axis::label::e_y;
    case BinningValue::binZ:
      return detray::axis::label::e_z;
    case BinningValue::binR:
      return detray::axis::label::e_r;
    case BinningValue::binPhi:
      return detray::axis::label::e_phi;
    case BinningValue::binRPhi:
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
  axis.label = convertBinningValue(bData.binvalue);
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
