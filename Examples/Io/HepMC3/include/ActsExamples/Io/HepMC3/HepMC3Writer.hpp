// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsExamples/Utilities/OptionsFwd.hpp"

#include <string>

#include <HepMC3/GenEvent.h>
#include <HepMC3/WriterAscii.h>

namespace ActsExamples {

/// HepMC3 event writer.
class HepMC3AsciiWriter final : public WriterT<std::vector<HepMC3::GenEvent>> {
 public:
  struct Config {
    // The output directory
    std::string outputDir;
    // The stem of output file names
    std::string outputStem;
    // The input collection
    std::string inputEvents;
  };

  /// Construct the writer.
  ///
  /// @param [in] config Config of the writer
  /// @param [in] level The level of the logger
  HepMC3AsciiWriter(const Config& config, Acts::Logging::Level level);

  /// Writing events to file.
  ///
  /// @param [in] ctx The context of this algorithm
  /// @param [in] events The recorded HepMC3 events
  ///
  /// @return Code describing whether the writing was successful
  ProcessCode writeT(const ActsExamples::AlgorithmContext& ctx,
                     const std::vector<HepMC3::GenEvent>& events) override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  /// The configuration of this writer
  Config m_cfg;
};

}  // namespace ActsExamples
