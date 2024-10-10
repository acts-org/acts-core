// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/ContextualDetector/AlignmentDecorator.hpp"
#include "ActsExamples/ContextualDetector/ExternallyAlignedDetectorElement.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Acts {
class TrackingGeometry;
}

namespace ActsExamples {
struct AlgorithmContext;

namespace Contextual {

/// @brief A mockup service that rotates the modules in a
/// simple tracking geometry
///
/// It acts on the PayloadDetectorElement, i.e. the
/// geometry context carries the full transform store (payload)
class ExternalAlignmentDecorator : public AlignmentDecorator {
 public:
  /// @brief nested configuration struct
  struct Config : public AlignmentDecorator::Config {
    /// The trackng geometry
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry = nullptr;
  };

  /// Constructor
  ///
  /// @param cfg Configuration struct
  /// @param logger The logging framework
  ExternalAlignmentDecorator(
      const Config& cfg,
      std::unique_ptr<const Acts::Logger> logger = Acts::getDefaultLogger(
          "ExternalAlignmentDecorator", Acts::Logging::INFO));

  /// Virtual destructor
  ~ExternalAlignmentDecorator() override = default;

  /// @brief decorates (adds, modifies) the AlgorithmContext
  /// with a geometric rotation per event
  ///
  /// @note If decorators depend on each other, they have to be
  /// added in order.
  ///
  /// @param context the bare (or at least non-const) Event context
  ProcessCode decorate(AlgorithmContext& context) override;

  /// @brief decorator name() for screen output
  const std::string& name() const override { return m_name; }

 private:
  Config m_cfg;                                  ///< the configuration class
  std::unique_ptr<const Acts::Logger> m_logger;  ///!< the logging instance
  std::string m_name = "ExternalAlignmentDecorator";

  /// Map of nominal transforms
  std::vector<Acts::Transform3> m_nominalStore;

  std::unordered_map<
      unsigned int,
      std::shared_ptr<ExternallyAlignedDetectorElement::AlignmentStore>>
      m_activeIovs;

  std::mutex m_iovMutex;

  std::size_t m_eventsSeen{0};

  /// Private access to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// Populate the nominal transforms
  /// this parses the TrackingGeometry and fills the nominal store
  ///
  /// @param tGeometry the tracking geometry
  void parseGeometry(const Acts::TrackingGeometry& tGeometry);
};
}  // namespace Contextual

}  // namespace ActsExamples
