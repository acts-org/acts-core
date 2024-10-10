// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <cstddef>
#include <limits>
#include <string>

namespace ActsExamples {
struct AlgorithmContext;

/// Write out a simhit collection before detector digitization in comma-
/// separated-value format.
///
/// This writes one file per event containing information about the
/// global space points, momenta (before and after interaction) and hit index
/// into the configured output directory. By default it writes to the
/// current working directory. Files are named using the following schema
///
///     event000000001-<stem>.csv
///     event000000002-<stem>.csv
///     ...
///
/// and each line in the file corresponds to one simhit.
class CsvSimHitWriter final : public WriterT<SimHitContainer> {
 public:
  struct Config {
    /// Which simulated (truth) hits collection to use.
    std::string inputSimHits;
    /// Where to place output files
    std::string outputDir;
    /// Output filename stem.
    std::string outputStem;
    /// Number of decimal digits for floating point precision in output.
    std::size_t outputPrecision = std::numeric_limits<float>::max_digits10;
  };

  /// Construct the cluster writer.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  CsvSimHitWriter(const Config& config, Acts::Logging::Level level);

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 protected:
  /// Type-specific write implementation.
  ///
  /// @param[in] ctx is the algorithm context
  /// @param[in] simHits are the simhits to be written
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const SimHitContainer& simHits) override;

 private:
  Config m_cfg;
};

}  // namespace ActsExamples
