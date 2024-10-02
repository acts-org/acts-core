// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/ContextualDetector/AlignedDetector.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/ILayerBuilder.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/ContextualDetector/AlignmentDecorator.hpp"
#include "ActsExamples/ContextualDetector/ExternalAlignmentDecorator.hpp"
#include "ActsExamples/ContextualDetector/ExternallyAlignedDetectorElement.hpp"
#include "ActsExamples/ContextualDetector/InternalAlignmentDecorator.hpp"
#include "ActsExamples/ContextualDetector/InternallyAlignedDetectorElement.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/GenericDetector/BuildGenericDetector.hpp"
#include "ActsExamples/GenericDetector/ProtoLayerCreatorT.hpp"

using namespace Acts::UnitLiterals;
namespace ActsExamples::Contextual {

auto AlignedDetector::finalize(
    const Config& cfg,
    std::shared_ptr<const Acts::IMaterialDecorator> mdecorator)
    -> std::pair<TrackingGeometryPtr, ContextDecorators> {
  ContextDecorators aContextDecorators;

  // Let's create a random number service
  ActsExamples::RandomNumbers::Config randomNumberConfig;
  randomNumberConfig.seed = cfg.seed;
  auto randomNumberSvc =
      std::make_shared<ActsExamples::RandomNumbers>(randomNumberConfig);

  auto fillDecoratorConfig = [&](AlignmentDecorator::Config& config) {
    config.iovSize = cfg.iovSize;
    config.flushSize = cfg.flushSize;
    config.doGarbageCollection = cfg.doGarbageCollection;

    // The misalignments
    config.gSigmaX = cfg.sigmaInPlane;
    config.gSigmaY = cfg.sigmaInPlane;
    config.gSigmaZ = cfg.sigmaOutPlane;
    config.aSigmaX = cfg.sigmaOutRot;
    config.aSigmaY = cfg.sigmaOutRot;
    config.aSigmaZ = cfg.sigmaInRot;
    config.randomNumberSvc = randomNumberSvc;
    config.firstIovNominal = cfg.firstIovNominal;
  };

  TrackingGeometryPtr aTrackingGeometry;
  if (cfg.mode == Config::Mode::External) {
    ExternallyAlignedDetectorElement::ContextType nominalContext;

    ExternalAlignmentDecorator::Config agcsConfig;
    fillDecoratorConfig(agcsConfig);

    std::vector<std::vector<std::shared_ptr<ExternallyAlignedDetectorElement>>>
        detectorStore;

    aTrackingGeometry =
        ActsExamples::Generic::buildDetector<ExternallyAlignedDetectorElement>(
            nominalContext, detectorStore, cfg.buildLevel,
            std::move(mdecorator), cfg.buildProto, cfg.surfaceLogLevel,
            cfg.layerLogLevel, cfg.volumeLogLevel);

    agcsConfig.trackingGeometry = aTrackingGeometry;

    // need to upcast to store in this object as well
    for (auto& lstore : detectorStore) {
      auto& target = m_detectorStore.emplace_back();
      for (auto& ldet : lstore) {
        target.push_back(ldet);
      }
    }

    aContextDecorators.push_back(std::make_shared<ExternalAlignmentDecorator>(
        std::move(agcsConfig),
        Acts::getDefaultLogger("AlignmentDecorator", cfg.decoratorLogLevel)));
  } else {
    InternallyAlignedDetectorElement::ContextType nominalContext;
    nominalContext.nominal = true;

    InternalAlignmentDecorator::Config agcsConfig;
    fillDecoratorConfig(agcsConfig);

    aTrackingGeometry =
        ActsExamples::Generic::buildDetector<InternallyAlignedDetectorElement>(
            nominalContext, agcsConfig.detectorStore, cfg.buildLevel,
            std::move(mdecorator), cfg.buildProto, cfg.surfaceLogLevel,
            cfg.layerLogLevel, cfg.volumeLogLevel);

    // need to upcast to store in this object as well
    for (auto& lstore : agcsConfig.detectorStore) {
      auto& target = m_detectorStore.emplace_back();
      for (auto& ldet : lstore) {
        target.push_back(ldet);
      }
    }

    aContextDecorators.push_back(std::make_shared<InternalAlignmentDecorator>(
        std::move(agcsConfig),
        Acts::getDefaultLogger("AlignmentDecorator", cfg.decoratorLogLevel)));
  }

  // return the pair of geometry and the alignment decorator(s)
  return std::make_pair<TrackingGeometryPtr, ContextDecorators>(
      std::move(aTrackingGeometry), std::move(aContextDecorators));
}

}  // namespace ActsExamples::Contextual
