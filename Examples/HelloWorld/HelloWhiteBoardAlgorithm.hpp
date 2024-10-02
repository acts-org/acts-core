// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
