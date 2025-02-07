// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/Framework/IAlgorithm.hpp"

#include <memory>

namespace ActsExamples {

/// A simple algorithm that just prints hello world.
class HelloLoggerAlgorithm : public ActsExamples::IAlgorithm {
 public:
  explicit HelloLoggerAlgorithm(Acts::Logging::Level level);

  // Log a few messages.
  ActsExamples::ProcessCode execute(const AlgorithmContext& ctx) const override;
};

}  // namespace ActsExamples
