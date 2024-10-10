// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/GenericDetector/GenericDetector.hpp"

#include "Acts/Geometry/ILayerBuilder.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "ActsExamples/GenericDetector/BuildGenericDetector.hpp"
#include "ActsExamples/GenericDetector/GenericDetectorElement.hpp"
#include "ActsExamples/GenericDetector/ProtoLayerCreatorT.hpp"

auto GenericDetector::finalize(
    const Config& cfg,
    std::shared_ptr<const Acts::IMaterialDecorator> mdecorator)
    -> std::pair<TrackingGeometryPtr, ContextDecorators> {
  DetectorElement::ContextType nominalContext;
  /// Return the generic detector
  TrackingGeometryPtr gGeometry =
      ActsExamples::Generic::buildDetector<DetectorElement>(
          nominalContext, detectorStore, cfg.buildLevel, std::move(mdecorator),
          cfg.buildProto, cfg.surfaceLogLevel, cfg.layerLogLevel,
          cfg.volumeLogLevel);
  ContextDecorators gContextDecorators = {};
  // return the pair of geometry and empty decorators
  return std::make_pair<TrackingGeometryPtr, ContextDecorators>(
      std::move(gGeometry), std::move(gContextDecorators));
}
