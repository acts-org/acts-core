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
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <limits>
#include <string>

namespace ActsExamples {
struct AlgorithmContext;

/// @class CsvMeasurementWriter
///
/// This writes multiples file per event containing information about the
/// measurement, the associated truth information and the cell/channel details
///
///     event000000001-cells.csv
///     event000000001-measurements.csv
///     event000000002-cells.csv
///     event000000002-measurements.csv
///     ...
///
/// Intrinsically thread-safe as one file per event.
class CsvMeasurementWriter final : public WriterT<MeasurementContainer> {
 public:
  struct Config {
    /// Which measurement collection to write.
    std::string inputMeasurements;
    /// Which cluster collection to write (optional)
    std::string inputClusters = "";
    /// Input collection to map measured hits to simulated hits.
    std::string inputMeasurementSimHitsMap;
    /// Where to place output files
    std::string outputDir;
    /// Number of decimal digits for floating point precision in output.
    int outputPrecision = std::numeric_limits<float>::max_digits10;
  };

  /// Constructor with
  /// @param config configuration struct
  /// @param level logging level
  CsvMeasurementWriter(const Config& config, Acts::Logging::Level level);

  /// Virtual destructor
  ~CsvMeasurementWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// This implementation holds the actual writing method
  /// and is called by the WriterT<>::write interface
  ///
  /// @param ctx The Algorithm context with per event information
  /// @param measurements is the data to be written out
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const MeasurementContainer& measurements) override;

 private:
  Config m_cfg;

  ReadDataHandle<IndexMultimap<Index>> m_inputMeasurementSimHitsMap{
      this, "InputMeasurementSimHitsMap"};
  ReadDataHandle<ClusterContainer> m_inputClusters{this, "InputClusters"};
};

}  // namespace ActsExamples
