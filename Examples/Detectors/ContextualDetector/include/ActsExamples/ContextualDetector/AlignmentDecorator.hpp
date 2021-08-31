// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/ContextualDetector/InternallyAlignedDetectorElement.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"

#include <mutex>
#include <vector>

namespace ActsExamples {

namespace Contextual {

/// @brief A mockup service that rotates the modules in a
/// simple tracking geometry
///
/// It acts on the PayloadDetectorElement, i.e. the
/// geometry context carries the full transform store (payload)
class AlignmentDecorator : public IContextDecorator {
 public:
  using LayerStore =
      std::vector<std::shared_ptr<InternallyAlignedDetectorElement>>;
  using DetectorStore = std::vector<LayerStore>;

  /// @brief nested configuration struct
  struct Config {
    /// The detector store (filled at creation creation)
    DetectorStore detectorStore;

    /// Alignment frequency - every X events
    unsigned int iovSize = 100;

    /// Flush store size - garbage collection
    unsigned int flushSize = 200;

    std::shared_ptr<RandomNumbers> randomNumberSvc = nullptr;

    /// Gaussian module parameters - 6 Degrees of freedom
    double gSigmaX = 0.;  // smear position along local x Axis
    double gSigmaY = 0.;  // smear position along local y Axis
    double gSigmaZ = 0.;  // smear position along local z Axis
    double aSigmaX = 0.;  // rotate around local x Axis
    double aSigmaY = 0.;  // rotate around local y Axis
    double aSigmaZ = 0.;  // rotate around local z Axis

    bool firstIovNominal = false;
  };

  /// Constructor
  ///
  /// @param cfg Configuration struct
  /// @param logger The logging framework
  AlignmentDecorator(const Config& cfg,
                     std::unique_ptr<const Acts::Logger> logger =
                         Acts::getDefaultLogger("AlignmentDecorator",
                                                Acts::Logging::INFO));

  /// Virtual destructor
  virtual ~AlignmentDecorator() = default;

  /// @brief decorates (adds, modifies) the AlgorithmContext
  /// with a geometric rotation per event
  ///
  /// @note If decorators depend on each other, they have to be
  /// added in order.
  ///
  /// @param context the bare (or at least non-const) Event context
  ProcessCode decorate(AlgorithmContext& context) final override;

  /// @brief decorator name() for screen output
  const std::string& name() const final override { return m_name; }

 private:
  Config m_cfg;                                  ///< the configuration class
  std::unique_ptr<const Acts::Logger> m_logger;  ///!< the logging instance
  std::string m_name = "AlignmentDecorator";

  ///< Protect multiple alignments to be loaded at once
  std::mutex m_alignmentMutex;
  std::vector<bool> m_iovStatus;
  std::vector<bool> m_flushStatus;

  /// Private access to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }
};
}  // namespace Contextual
}  // namespace ActsExamples
