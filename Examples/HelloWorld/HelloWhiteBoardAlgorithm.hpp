// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <memory>

#include "HelloData.hpp"

namespace ActsExamples {

/// Example algorithm that reads/writes data from/to the event store.
class HelloWhiteBoardAlgorithm : public ActsExamples::IAlgorithm {
 public:
  struct Config {
    /// Input collection name.
    std::string input;
    /// Output collection name.
    std::string output;
  };

  HelloWhiteBoardAlgorithm(const Config& cfg,
                           Acts::Logging::Level level = Acts::Logging::INFO);

  /// Read input and copy to the output
  ActsExamples::ProcessCode execute(const AlgorithmContext& ctx) const override;

  ReadDataHandle<HelloDataCollection> m_readHandle{this, "Input"};
  WriteDataHandle<HelloDataCollection> m_writeHandle{this, "Output"};

 private:
  Config m_cfg;
};

}  // namespace ActsExamples
