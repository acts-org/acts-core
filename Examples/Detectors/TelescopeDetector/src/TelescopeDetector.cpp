// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TelescopeDetector/TelescopeDetector.hpp"

#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "ActsExamples/TelescopeDetector/BuildTelescopeDetector.hpp"
#include "ActsExamples/TelescopeDetector/TelescopeDetectorElement.hpp"

#include <algorithm>
#include <stdexcept>

auto ActsExamples::Telescope::TelescopeDetector::finalize(
    const Config& cfg, const std::shared_ptr<const Acts::IMaterialDecorator>&
    /*mdecorator*/) -> std::pair<TrackingGeometryPtr, ContextDecorators> {
  DetectorElement::ContextType nominalContext;

  if (cfg.surfaceType > 1) {
    throw std::invalid_argument(
        "The surface type could either be 0 for plane surface or 1 for disc "
        "surface.");
  }
  if (cfg.binValue > 2) {
    throw std::invalid_argument("The axis value could only be 0, 1, or 2.");
  }
  // Check if the bounds values are valid
  if (cfg.surfaceType == 1 && cfg.bounds[0] >= cfg.bounds[1]) {
    throw std::invalid_argument(
        "The minR should be smaller than the maxR for disc surface bounds.");
  }

  if (cfg.positions.size() != cfg.stereos.size()) {
    throw std::invalid_argument(
        "The number of provided positions must match the number of "
        "provided stereo angles.");
  }

  config = cfg;

  // Sort the provided distances
  std::vector<double> positions = cfg.positions;
  std::vector<double> stereos = cfg.stereos;
  std::ranges::sort(positions);

  /// Return the telescope detector
  TrackingGeometryPtr gGeometry = ActsExamples::Telescope::buildDetector(
      nominalContext, detectorStore, positions, stereos, cfg.offsets,
      cfg.bounds, cfg.thickness,
      static_cast<ActsExamples::Telescope::TelescopeSurfaceType>(
          cfg.surfaceType),
      static_cast<Acts::BinningValue>(cfg.binValue));
  ContextDecorators gContextDecorators = {};
  // return the pair of geometry and empty decorators
  return std::make_pair<TrackingGeometryPtr, ContextDecorators>(
      std::move(gGeometry), std::move(gContextDecorators));
}
