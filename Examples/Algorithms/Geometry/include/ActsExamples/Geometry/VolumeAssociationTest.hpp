// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"

namespace Acts::Experimental {
class Detector;
}  // namespace Acts::Experimental

namespace ActsExamples {

struct AlgorithmContext;

/// This is a test algorithm that checks the unique volume identification
/// and association for Detector objects
class VolumeAssociationTest final : public IAlgorithm {
 public:
  /// Nested Configuration struct
  struct Config {
    /// Name of the object
    std::string name = "VolumeAssociationTets";
    /// Number of tests
    std::size_t ntests = 1000;
    /// The random number service
    std::shared_ptr<const RandomNumbers> randomNumbers = nullptr;
    /// The random number range
    std::vector<Acts::ActsScalar> randomRange = {};
    /// The detector
    std::shared_ptr<const Acts::Experimental::Detector> detector = nullptr;
  };

  /// Construct the  volume association test algorithm
  ///
  /// @param cfg is the algorithm configuration
  /// @param level is the logging level
  VolumeAssociationTest(const Config& cfg,
                        Acts::Logging::Level level = Acts::Logging::INFO);

  /// Run the random point association test
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const override;

  /// Const access to the config
  const Config& config() const { return m_cfg; }

 private:
  /// The algorithm configuration.
  Config m_cfg;
};

}  // namespace ActsExamples
