// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <utility>

namespace ActsExamples {

class WhiteBoard;

/// Event data reader that takes a concrete reader instance, reads a number of
/// events in a buffer, and selects events from that buffer instead of directly
/// reading them from disk.
/// The purpose is to avoid IO bottlenecks in timing measurements
class BufferedReader final : public IReader {
 public:
  struct Config {
    /// The upstream reader that should be used
    std::shared_ptr<IReader> upstreamReader;

    /// The seed for sampling events from the buffer
    std::size_t selectionSeed = 123456;

    /// Buffer size. The reader will throw and exception if the downstream
    /// reader does not provide enough events
    std::size_t bufferSize = 1;
  };

  /// Constructed the reader
  BufferedReader(const Config& config, Acts::Logging::Level level);

  /// Return the config
  const Config& config() const { return m_cfg; }

  /// Give the reader a understandable name
  std::string name() const override {
    return "Buffered" + m_cfg.upstreamReader->name();
  }

  /// The buffered reader provides the maximum available event range
  std::pair<std::size_t, std::size_t> availableEvents() const override {
    return {0, std::numeric_limits<std::size_t>::max()};
  }

  /// Return a event from the buffer
  ProcessCode read(const AlgorithmContext& ctx) override;

  /// Fulfill the algorithm interface
  ProcessCode initialize() override { return ProcessCode::SUCCESS; }

  /// Fulfill the algorithm interface
  ProcessCode finalize() override { return ProcessCode::SUCCESS; }

 private:
  Config m_cfg;
  std::unique_ptr<const Acts::Logger> m_logger;
  std::vector<std::unique_ptr<ActsExamples::WhiteBoard>> m_buffer;

  const Acts::Logger& logger() const { return *m_logger; }
};

}  // namespace ActsExamples
