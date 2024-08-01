// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GenericDetector/GenericDetector.hpp"

#include "Acts/Geometry/ILayerBuilder.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Plugins/GenericDetector/BuildGenericDetector.hpp"
#include "Acts/Plugins/GenericDetector/GenericDetectorElement.hpp"
#include "Acts/Plugins/GenericDetector/ProtoLayerCreatorT.hpp"

namespace Acts {
auto GenericDetector::finalize(
    const Config& cfg,
    std::shared_ptr<const Acts::IMaterialDecorator> mdecorator)
    -> TrackingGeometryPtr {
  DetectorElement::ContextType nominalContext;
  /// Return the generic detector
  TrackingGeometryPtr gGeometry = Acts::Generic::buildDetector<DetectorElement>(
      nominalContext, detectorStore, cfg.buildLevel, std::move(mdecorator),
      cfg.buildProto, cfg.surfaceLogLevel, cfg.layerLogLevel,
      cfg.volumeLogLevel);
  // return the pair of geometry and empty decorators
  return gGeometry;
}
}  // namespace Acts
