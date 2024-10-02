// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IReader.hpp"

#include <HepMC3/GenEvent.h>
#include <HepMC3/ReaderAscii.h>

namespace ActsExamples {

/// HepMC3 event reader.
class HepMC3AsciiReader final : public IReader {
 public:
  struct Config {
    // The input directory
    std::string inputDir;
    // The stem of input file names
    std::string inputStem;
    // The output collection
    std::string outputEvents;
  };

  /// @brief Reads an event from file
  /// @param reader reader of run files
  /// @param event storage of the read event
  /// @return boolean indicator if the reading was successful
  bool readEvent(HepMC3::ReaderAscii& reader, HepMC3::GenEvent& event);

  /// @brief Reports the status of the reader
  /// @param reader reader of run files
  /// @return boolean status indicator
  bool status(HepMC3::ReaderAscii& reader);

  /// Construct the particle reader.
  ///
  /// @param [in] cfg The configuration object
  /// @param [in] lvl The logging level
  HepMC3AsciiReader(const Config& cfg, Acts::Logging::Level lvl);

  std::string name() const override;

  /// Return the available events range.
  std::pair<std::size_t, std::size_t> availableEvents() const override;

  /// Read out data from the input stream.
  ProcessCode read(const ActsExamples::AlgorithmContext& ctx) override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  /// The configuration of this writer
  Config m_cfg;
  /// Number of events
  std::pair<std::size_t, std::size_t> m_eventsRange;
  /// The logger
  std::unique_ptr<const Acts::Logger> m_logger;

  const Acts::Logger& logger() const { return *m_logger; }

  WriteDataHandle<std::vector<HepMC3::GenEvent>> m_outputEvents{this,
                                                                "OutputEvents"};
};

}  // namespace ActsExamples
