// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/ContextualDetector/AlignedDetector.hpp"

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/ContextualDetector/AlignmentDecorator.hpp"
#include "ActsExamples/ContextualDetector/ExternalAlignmentDecorator.hpp"
#include "ActsExamples/ContextualDetector/ExternallyAlignedDetectorElement.hpp"
#include "ActsExamples/ContextualDetector/InternalAlignmentDecorator.hpp"
#include "ActsExamples/ContextualDetector/InternallyAlignedDetectorElement.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/GenericDetector/BuildGenericDetector.hpp"

#include <memory>

namespace ActsExamples {

AlignedDetector::AlignedDetector(const Config& cfg)
    : Detector(Acts::getDefaultLogger("AlignedDetector", cfg.logLevel)),
      m_cfg(cfg) {
  if (m_cfg.mode == Config::Mode::External) {
    InternallyAlignedDetectorElement::ContextType nominalContext;
    m_nominalGeometryContext = Acts::GeometryContext(nominalContext);
  } else {
    InternallyAlignedDetectorElement::ContextType nominalContext;
    nominalContext.nominal = true;
    m_nominalGeometryContext = Acts::GeometryContext(nominalContext);
  }

  // Let's create a random number service
  RandomNumbers::Config randomNumberConfig;
  randomNumberConfig.seed = m_cfg.seed;
  auto randomNumberSvc = std::make_shared<RandomNumbers>(randomNumberConfig);

  auto fillDecoratorConfig = [&](AlignmentDecorator::Config& config) {
    config.iovSize = m_cfg.iovSize;
    config.flushSize = m_cfg.flushSize;
    config.doGarbageCollection = m_cfg.doGarbageCollection;

    // The misalignments
    config.gSigmaX = m_cfg.sigmaInPlane;
    config.gSigmaY = m_cfg.sigmaInPlane;
    config.gSigmaZ = m_cfg.sigmaOutPlane;
    config.aSigmaX = m_cfg.sigmaOutRot;
    config.aSigmaY = m_cfg.sigmaOutRot;
    config.aSigmaZ = m_cfg.sigmaInRot;
    config.randomNumberSvc = randomNumberSvc;
    config.firstIovNominal = m_cfg.firstIovNominal;
  };

  if (m_cfg.mode == Config::Mode::External) {
    ExternalAlignmentDecorator::Config agcsConfig;
    fillDecoratorConfig(agcsConfig);

    std::vector<std::vector<std::shared_ptr<ExternallyAlignedDetectorElement>>>
        specificDetectorStore;

    m_trackingGeometry =
        Generic::buildDetector<ExternallyAlignedDetectorElement>(
            m_nominalGeometryContext, specificDetectorStore, m_cfg.buildLevel,
            m_cfg.materialDecorator, m_cfg.buildProto, m_cfg.surfaceLogLevel,
            m_cfg.layerLogLevel, m_cfg.volumeLogLevel);
    agcsConfig.trackingGeometry = m_trackingGeometry;

    // need to upcast to store in this object as well
    for (auto& lstore : specificDetectorStore) {
      for (auto& ldet : lstore) {
        m_detectorStore.push_back(ldet);
      }
    }

    m_contextDecorators.push_back(std::make_shared<ExternalAlignmentDecorator>(
        std::move(agcsConfig),
        Acts::getDefaultLogger("AlignmentDecorator", m_cfg.decoratorLogLevel)));
  } else {
    InternalAlignmentDecorator::Config agcsConfig;
    fillDecoratorConfig(agcsConfig);

    m_trackingGeometry =
        Generic::buildDetector<InternallyAlignedDetectorElement>(
            m_nominalGeometryContext, agcsConfig.detectorStore,
            m_cfg.buildLevel, m_cfg.materialDecorator, m_cfg.buildProto,
            m_cfg.surfaceLogLevel, m_cfg.layerLogLevel, m_cfg.volumeLogLevel);

    // need to upcast to store in this object as well
    for (auto& lstore : agcsConfig.detectorStore) {
      for (auto& ldet : lstore) {
        m_detectorStore.push_back(ldet);
      }
    }

    m_contextDecorators.push_back(std::make_shared<InternalAlignmentDecorator>(
        std::move(agcsConfig),
        Acts::getDefaultLogger("AlignmentDecorator", m_cfg.decoratorLogLevel)));
  }
}

}  // namespace ActsExamples
