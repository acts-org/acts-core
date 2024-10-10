// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/TrackingGeometryBuilder.hpp"

#include "Acts/Geometry/TrackingGeometry.hpp"

#include <functional>
#include <stdexcept>
#include <utility>

Acts::TrackingGeometryBuilder::TrackingGeometryBuilder(
    const Acts::TrackingGeometryBuilder::Config& cgbConfig,
    std::unique_ptr<const Logger> logger)
    : m_cfg(), m_logger(std::move(logger)) {
  setConfiguration(cgbConfig);
}

const Acts::TrackingGeometryBuilder::Config&
Acts::TrackingGeometryBuilder::getConfiguration() const {
  return m_cfg;
}

void Acts::TrackingGeometryBuilder::setConfiguration(
    const Acts::TrackingGeometryBuilder::Config& cgbConfig) {
  if (cgbConfig.trackingVolumeBuilders.empty()) {
    throw std::invalid_argument("Invalid configuration: no volume builders");
  }
  m_cfg = cgbConfig;
}

void Acts::TrackingGeometryBuilder::setLogger(
    std::unique_ptr<const Logger> newLogger) {
  m_logger = std::move(newLogger);
}

std::unique_ptr<const Acts::TrackingGeometry>
Acts::TrackingGeometryBuilder::trackingGeometry(
    const GeometryContext& gctx) const {
  MutableTrackingVolumePtr highestVolume = nullptr;
  // loop over the builders and wrap one around the other
  for (auto& volumeBuilder : m_cfg.trackingVolumeBuilders) {
    // assign a new highest volume (and potentially wrap around the given
    // highest volume so far)
    auto volume = volumeBuilder(gctx, highestVolume, nullptr);
    if (!volume) {
      ACTS_INFO(
          "Received nullptr volume from builder, keeping previous highest "
          "volume");
    } else {
      highestVolume = std::move(volume);
    }
  }

  // create the TrackingGeometry & decorate it with the material
  if (highestVolume) {
    return std::make_unique<TrackingGeometry>(
        highestVolume,
        m_cfg.materialDecorator ? m_cfg.materialDecorator.get() : nullptr,
        *m_cfg.geometryIdentifierHook, logger());
  } else {
    throw std::runtime_error(
        "Unable to construct tracking geometry: no tracking volume");
  }
}
