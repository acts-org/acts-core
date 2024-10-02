// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/ContextualDetector/AlignmentDecorator.hpp"
#include "ActsExamples/ContextualDetector/InternallyAlignedDetectorElement.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ActsExamples {
struct AlgorithmContext;

namespace Contextual {
class InternallyAlignedDetectorElement;

/// @brief A mockup service that rotates the modules in a
/// simple tracking geometry
///
/// It acts on the PayloadDetectorElement, i.e. the
/// geometry context carries the full transform store (payload)
class InternalAlignmentDecorator : public AlignmentDecorator {
 public:
  using LayerStore =
      std::vector<std::shared_ptr<InternallyAlignedDetectorElement>>;
  using DetectorStore = std::vector<LayerStore>;

  /// @brief nested configuration struct
  struct Config : public AlignmentDecorator::Config {
    /// The detector store (filled at creation creation)
    DetectorStore detectorStore;
  };

  /// Constructor
  ///
  /// @param cfg Configuration struct
  /// @param logger The logging framework
  InternalAlignmentDecorator(const Config& cfg,
                             std::unique_ptr<const Acts::Logger> logger =
                                 Acts::getDefaultLogger("AlignmentDecorator",
                                                        Acts::Logging::INFO));

  /// Virtual destructor
  ~InternalAlignmentDecorator() override = default;

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
  std::string m_name = "AlignmentDecorator";

  ///< Protect multiple alignments to be loaded at once
  std::mutex m_alignmentMutex;
  struct IovStatus {
    std::size_t lastAccessed;
  };
  std::unordered_map<unsigned int, IovStatus> m_activeIovs;
  std::size_t m_eventsSeen{0};

  /// Private access to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }
};
}  // namespace Contextual
}  // namespace ActsExamples
