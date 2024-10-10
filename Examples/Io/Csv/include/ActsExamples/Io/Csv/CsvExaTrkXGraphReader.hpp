// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Graph.hpp"
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

class CsvExaTrkXGraphReader final : public IReader {
 public:
  struct Config {
    /// Where to read input files from.
    std::string inputDir;
    /// Input filename stem.
    std::string inputStem;
    /// Which vs collection to read into.
    std::string outputGraph;
  };

  /// Construct the particle reader.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  CsvExaTrkXGraphReader(const Config& config, Acts::Logging::Level level);

  /// Return the available events range.
  std::pair<std::size_t, std::size_t> availableEvents() const override;

  /// Read out data from the input stream.
  ProcessCode read(const ActsExamples::AlgorithmContext& ctx) override;

  /// Return the name of the component
  std::string name() const override { return "CsvExaTrkXGraphReader"; }

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  std::pair<std::size_t, std::size_t> m_eventsRange;
  std::unique_ptr<const Acts::Logger> m_logger;

  WriteDataHandle<Graph> m_outputGraph{this, "OutputGraph"};
  const Acts::Logger& logger() const { return *m_logger; }
};

}  // namespace ActsExamples
