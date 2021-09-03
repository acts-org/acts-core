// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/Detector/IBaseDetector.hpp"
#include "ActsExamples/GenericDetector/GenericDetector.hpp"
#include "ActsExamples/Utilities/OptionsFwd.hpp"

#include <memory>
#include <vector>

namespace ActsExamples::Contextual {
class InternallyAlignedDetectorElement;
class InternalAlignmentDecorator;

class AlignedDetector : public ActsExamples::IBaseDetector {
 public:
  //   using DetectorElement =
  //       ActsExamples::Contextual::InternallyAlignedDetectorElement;
  //   using DetectorElementPtr = std::shared_ptr<DetectorElement>;
  //   using Decorator = ActsExamples::Contextual::InternalAlignmentDecorator;
  //   using DetectorStore = std::vector<std::vector<DetectorElementPtr>>;

  struct Config : public GenericDetector::Config {
    /// Seed for the decorator random numbers.
    double seed = 1324354657;
    /// Size of a valid IOV.
    size_t iovSize = 100;
    /// Span until garbage collection is active.
    size_t flushSize = 200;
    bool doGarbageCollection = true;
    /// Sigma of the in-plane misalignment
    double sigmaInPlane = 100 * Acts::UnitConstants::um;
    /// Sigma of the out-of-plane misalignment
    double sigmaOutPlane = 50 * Acts::UnitConstants::um;
    /// Sigma of the in-plane rotation misalignment
    double sigmaInRot = 20 * 0.001;  // millirad
    /// Sigma of the out-of-plane rotation misalignment
    double sigmaOutRot = 0;
    /// Keep the first iov batch nominal.
    bool firstIovNominal = false;
    /// Log level for the decorator
    Acts::Logging::Level decoratorLogLevel = Acts::Logging::INFO;

    enum class Mode { Internal, External };
    Mode mode = Mode::Internal;
  };

  void addOptions(
      boost::program_options::options_description& opt) const override;

  std::pair<ActsExamples::IBaseDetector::TrackingGeometryPtr, ContextDecorators>
  finalize(const boost::program_options::variables_map& vm,
           std::shared_ptr<const Acts::IMaterialDecorator> mdecorator) override;

  std::pair<ActsExamples::IBaseDetector::TrackingGeometryPtr, ContextDecorators>
  finalize(const Config& cfg,
           std::shared_ptr<const Acts::IMaterialDecorator> mdecorator);

  std::vector<std::vector<std::shared_ptr<Generic::GenericDetectorElement>>>&
  detectorStore() {
    return m_detectorStore;
  }

 private:
  /// The Store of the detector elements (lifetime: job)
  std::vector<std::vector<std::shared_ptr<Generic::GenericDetectorElement>>>
      m_detectorStore;
};

}  // namespace ActsExamples::Contextual