// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/GenericDetector/GenericDetector.hpp"

#include "ActsExamples/GenericDetector/BuildGenericDetector.hpp"
#include "ActsExamples/GenericDetector/GenericDetectorElement.hpp"

namespace ActsExamples {

GenericDetector::GenericDetector(const Config& cfg)
    : Detector(Acts::getDefaultLogger("GenericDetector", cfg.logLevel)),
      m_cfg(cfg) {
  m_nominalGeometryContext = Acts::GeometryContext();

  std::vector<std::vector<std::shared_ptr<GenericDetectorElement>>>
      specificDetectorStore;
  m_trackingGeometry = Generic::buildDetector<GenericDetectorElement>(
      m_nominalGeometryContext, specificDetectorStore, m_cfg.buildLevel,
      m_cfg.materialDecorator, m_cfg.buildProto, m_cfg.surfaceLogLevel,
      m_cfg.layerLogLevel, m_cfg.volumeLogLevel);

  for (const auto& something : specificDetectorStore) {
    for (const auto& element : something) {
      m_detectorStore.push_back(element);
    }
  }
}

}  // namespace ActsExamples
