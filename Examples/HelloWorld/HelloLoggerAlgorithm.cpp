// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
