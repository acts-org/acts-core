// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/GenericDetector/GenericDetectorElement.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace Acts {

class TrackingGeometry;
class IMaterialDecorator;

struct GenericDetector {
  using DetectorElement = Generic::GenericDetectorElement;
  using DetectorElementPtr = std::shared_ptr<DetectorElement>;
  using DetectorStore = std::vector<std::vector<DetectorElementPtr>>;

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

  TrackingGeometryPtr finalize(
      const Config& cfg,
      std::shared_ptr<const Acts::IMaterialDecorator> mdecorator);
};

}  // namespace Acts
