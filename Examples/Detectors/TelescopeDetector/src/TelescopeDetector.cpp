// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TelescopeDetector/TelescopeDetector.hpp"

#include "Acts/Geometry/GeometryContext.hpp"
#include "ActsExamples/TelescopeDetector/BuildTelescopeDetector.hpp"

#include <stdexcept>

namespace ActsExamples {

TelescopeDetector::TelescopeDetector(const Config& cfg)
    : Detector(Acts::getDefaultLogger("TelescopeDetector", cfg.logLevel)),
      m_cfg(cfg) {
  if (m_cfg.surfaceType > 1) {
    throw std::invalid_argument(
        "The surface type could either be 0 for plane surface or 1 for disc "
        "surface.");
  }
  if (m_cfg.binValue > 2) {
    throw std::invalid_argument("The axis value could only be 0, 1, or 2.");
  }
  // Check if the bounds values are valid
  if (m_cfg.surfaceType == 1 && m_cfg.bounds[0] >= m_cfg.bounds[1]) {
    throw std::invalid_argument(
        "The minR should be smaller than the maxR for disc surface bounds.");
  }

  if (m_cfg.positions.size() != m_cfg.stereos.size()) {
    throw std::invalid_argument(
        "The number of provided positions must match the number of "
        "provided stereo angles.");
  }

  m_nominalGeometryContext = Acts::GeometryContext();

  m_trackingGeometry = buildTelescopeDetector(
      m_nominalGeometryContext, m_detectorStore, m_cfg.positions, m_cfg.stereos,
      m_cfg.offsets, m_cfg.bounds, m_cfg.thickness,
      static_cast<TelescopeSurfaceType>(m_cfg.surfaceType),
      static_cast<Acts::AxisDirection>(m_cfg.binValue));
}

}  // namespace ActsExamples
