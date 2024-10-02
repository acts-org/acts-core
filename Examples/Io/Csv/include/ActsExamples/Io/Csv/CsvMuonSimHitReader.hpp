// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace ActsExamples {
struct AlgorithmContext;

/// Read in a simhit collection in comma-separated-value format.
///
/// This reads one files per event in the configured input directory. By default
/// it reads files in the current working directory. Files are assumed to be
/// named using the following schema
///
///     event000000001-<stem>.csv
///     event000000002-<stem>.csv
///
/// and each line in the file corresponds to one simhit.
class CsvMuonSimHitReader final : public IReader {
 public:
  struct Config {
    /// Where to read input files from.
    std::string inputDir;
    /// Input filename stem.
    std::string inputStem;
    /// Output simulated (truth) hits collection.
    std::string outputSimHits;
  };

  /// Construct the simhit reader.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  CsvMuonSimHitReader(const Config& config, Acts::Logging::Level level);

  std::string name() const override;

  /// Return the available events range.
  std::pair<std::size_t, std::size_t> availableEvents() const override;

  /// Read out data from the input stream.
  ProcessCode read(const ActsExamples::AlgorithmContext& ctx) override;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  std::pair<std::size_t, std::size_t> m_eventsRange;
  std::unique_ptr<const Acts::Logger> m_logger;

  WriteDataHandle<SimHitContainer> m_outputSimHits{this, "OutputSimHits"};

  const Acts::Logger& logger() const { return *m_logger; }
};

}  // namespace ActsExamples
