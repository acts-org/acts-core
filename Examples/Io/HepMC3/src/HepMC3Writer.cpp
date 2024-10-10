// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/HepMC3/HepMC3Writer.hpp"

#include "ActsExamples/Utilities/Paths.hpp"

namespace ActsExamples {

HepMC3AsciiWriter::HepMC3AsciiWriter(const Config& config,
                                     Acts::Logging::Level level)
    : WriterT(config.inputEvents, "HepMC3AsciiWriter", level), m_cfg(config) {
  if (m_cfg.outputStem.empty()) {
    throw std::invalid_argument("Missing output stem file name");
  }
}

ProcessCode HepMC3AsciiWriter::writeT(
    const AlgorithmContext& ctx, const std::vector<HepMC3::GenEvent>& events) {
  auto path = perEventFilepath(m_cfg.outputDir, m_cfg.outputStem + ".hepmc3",
                               ctx.eventNumber);

  ACTS_DEBUG("Attempting to write event to " << path);
  HepMC3::WriterAscii writer(path);

  for (const auto& event : events) {
    writer.write_event(event);
    if (writer.failed()) {
      return ProcessCode::ABORT;
    }
  }

  writer.close();
  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
