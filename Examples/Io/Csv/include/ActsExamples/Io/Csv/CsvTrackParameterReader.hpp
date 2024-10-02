// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace ActsExamples {
struct AlgorithmContext;

/// Read track parameters in comma-separated-value format.
///
/// This reads one file per event in the configured input directory
/// and filename. Files are assumed to be named using the following schema
///
///     event000000001-<stem>.csv
///     event000000002-<stem>.csv
///
/// and each line in the file corresponds to one track parameter.
class CsvTrackParameterReader final : public IReader {
 public:
  struct Config {
    /// Where to read input files from.
    std::string inputDir;
    /// Input filename stem.
    std::string inputStem;
    /// Which vs collection to read into.
    std::string outputTrackParameters;

    /// Beamspot
    std::array<double, 3> beamspot{};
  };

  /// Construct the track parameter reader.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  CsvTrackParameterReader(const Config& config, Acts::Logging::Level level);

  std::string name() const final;

  /// Return the available events range.
  std::pair<std::size_t, std::size_t> availableEvents() const final;

  /// Read out data from the input stream.
  ProcessCode read(const ActsExamples::AlgorithmContext& ctx) final;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  std::pair<std::size_t, std::size_t> m_eventsRange;
  std::unique_ptr<const Acts::Logger> m_logger;

  WriteDataHandle<TrackParametersContainer> m_outputTrackParameters{
      this, "OutputTrackParameters"};

  const Acts::Logger& logger() const { return *m_logger; }
};

}  // namespace ActsExamples
