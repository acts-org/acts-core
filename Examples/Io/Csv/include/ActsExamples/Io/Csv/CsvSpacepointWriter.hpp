// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <string>

namespace ActsExamples {

/// @class CsvSpacePointWriter
///
/// This writes one file per event containing information about the
/// spacepoints
///
///     event000000001-spacepoint.csv
///     event000000002-spacepoint.csv
///     ...
///
/// Intrinsically thread-safe as one file per event.
class CsvSpacepointWriter final : public WriterT<SimSpacePointContainer> {
 public:
  struct Config {
    /// Which measurement collection to write.
    std::string inputSpacepoints;
    /// Where to place output files
    std::string outputDir;
    /// Number of decimal digits for floating point precision in output.
    std::size_t outputPrecision = std::numeric_limits<float>::max_digits10;
  };

  /// Constructor with
  /// @param config configuration struct
  /// @param level logging level
  CsvSpacepointWriter(const Config& config, Acts::Logging::Level level);

  /// Virtual destructor
  ~CsvSpacepointWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// This implementation holds the actual writing method
  /// and is called by the WriterT<>::write interface
  ///
  /// @param ctx The Algorithm context with per event information
  /// @param spacepoints is the data to be written out
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const SimSpacePointContainer& spacepoints) override;

 private:
  Config m_cfg;
};

}  // namespace ActsExamples
