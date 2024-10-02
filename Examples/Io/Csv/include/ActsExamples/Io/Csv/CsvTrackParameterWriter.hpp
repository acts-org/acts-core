// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IWriter.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace ActsExamples {
struct AlgorithmContext;

/// Write track parameters in comma-separated-value format.
///
/// This writes one file per event in the configured input directory
/// and filename. Files are assumed to be named using the following schema
///
///     event000000001-<stem>.csv
///     event000000002-<stem>.csv
///
/// and each line in the file corresponds to one track parameter.
class CsvTrackParameterWriter final : public IWriter {
 public:
  struct Config {
    /// Optional. Input track parameters collection
    std::string inputTrackParameters;
    /// Optional. Input track container.
    std::string inputTracks;
    /// Where to place output files
    std::string outputDir;
    /// Input filename stem.
    std::string outputStem;
    /// Number of decimal digits for floating point precision in output.
    std::size_t outputPrecision = std::numeric_limits<float>::max_digits10;
  };

  /// Constructor with
  /// @param config configuration struct
  /// @param level logging level
  CsvTrackParameterWriter(const Config& config, Acts::Logging::Level level);

  /// Virtual destructor
  ~CsvTrackParameterWriter() override;

  /// Provide the name of the writer
  std::string name() const override;

  /// Read the object and call the type-specific member function.
  ProcessCode write(const AlgorithmContext& ctx) override;

  /// No-op default implementation.
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  std::unique_ptr<const Acts::Logger> m_logger;

  ReadDataHandle<TrackParametersContainer> m_inputTrackParameters{
      this, "InputTrackParameters"};
  ReadDataHandle<ConstTrackContainer> m_inputTracks{this, "InputTracks"};
};

}  // namespace ActsExamples
