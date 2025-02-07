// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "HelloLoggerAlgorithm.hpp"

#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <cstddef>

ActsExamples::HelloLoggerAlgorithm::HelloLoggerAlgorithm(
    Acts::Logging::Level level)
    : ActsExamples::IAlgorithm("HelloLogger", level) {}

ActsExamples::ProcessCode ActsExamples::HelloLoggerAlgorithm::execute(
    const AlgorithmContext& ctx) const {
  ACTS_INFO(" Hello World! (from event=" << ctx.eventNumber);
  ACTS_DEBUG("  - that's an ACTS_DEBUG message");
  ACTS_VERBOSE("  - that's an ACTS_VERBOSE message");
  return ActsExamples::ProcessCode::SUCCESS;
}
