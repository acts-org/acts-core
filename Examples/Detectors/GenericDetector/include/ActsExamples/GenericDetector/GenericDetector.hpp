// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace Acts {
class TrackingGeometry;
class IMaterialDecorator;
}  // namespace Acts

namespace ActsExamples {
class IContextDecorator;
}  // namespace ActsExamples

namespace ActsExamples::Generic {
class GenericDetectorElement;
}  // namespace ActsExamples::Generic

struct GenericDetector {
  using DetectorElement = ActsExamples::Generic::GenericDetectorElement;
  using DetectorElementPtr = std::shared_ptr<DetectorElement>;
  using DetectorStore = std::vector<std::vector<DetectorElementPtr>>;

  using ContextDecorators =
      std::vector<std::shared_ptr<ActsExamples::IContextDecorator>>;
  using TrackingGeometryPtr = std::shared_ptr<const Acts::TrackingGeometry>;

  struct Config {
    std::size_t buildLevel{3};
    Acts::Logging::Level surfaceLogLevel{Acts::Logging::INFO};
    Acts::Logging::Level layerLogLevel{Acts::Logging::INFO};
    Acts::Logging::Level volumeLogLevel{Acts::Logging::INFO};
    bool buildProto{false};
  };

  /// The Store of the detector elements (lifetime: job)
  DetectorStore detectorStore;

  std::pair<TrackingGeometryPtr, ContextDecorators> finalize(
      const Config& cfg,
      std::shared_ptr<const Acts::IMaterialDecorator> mdecorator);
};
