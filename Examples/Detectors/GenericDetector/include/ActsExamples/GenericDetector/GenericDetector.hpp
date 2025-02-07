// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/DetectorCommons/Detector.hpp"

#include <cstddef>
#include <memory>

namespace Acts {
class IMaterialDecorator;
}

namespace ActsExamples {

class GenericDetectorElement;

class GenericDetector : public Detector {
 public:
  struct Config {
    std::size_t buildLevel = 3;
    Acts::Logging::Level logLevel = Acts::Logging::INFO;
    Acts::Logging::Level surfaceLogLevel = Acts::Logging::INFO;
    Acts::Logging::Level layerLogLevel = Acts::Logging::INFO;
    Acts::Logging::Level volumeLogLevel = Acts::Logging::INFO;
    bool buildProto = false;
    std::shared_ptr<const Acts::IMaterialDecorator> materialDecorator;
  };

  explicit GenericDetector(const Config& cfg);

 private:
  Config m_cfg;
};

}  // namespace ActsExamples
