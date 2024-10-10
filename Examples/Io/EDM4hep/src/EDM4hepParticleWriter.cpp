// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/EDM4hep/EDM4hepParticleWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepUtil.hpp"
#include "ActsExamples/Utilities/Paths.hpp"

#include <stdexcept>

#include <edm4hep/MCParticle.h>
#include <edm4hep/MCParticleCollection.h>
#include <podio/Frame.h>

namespace ActsExamples {

EDM4hepParticleWriter::EDM4hepParticleWriter(
    const EDM4hepParticleWriter::Config& cfg, Acts::Logging::Level lvl)
    : WriterT(cfg.inputParticles, "EDM4hepParticleWriter", lvl),
      m_cfg(cfg),
      m_writer(cfg.outputPath) {
  ACTS_VERBOSE("Created output file " << cfg.outputPath);

  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Missing particles input collection");
  }
}

ActsExamples::ProcessCode EDM4hepParticleWriter::finalize() {
  m_writer.finish();

  return ProcessCode::SUCCESS;
}

ProcessCode EDM4hepParticleWriter::writeT(
    const AlgorithmContext& /*ctx*/, const SimParticleContainer& particles) {
  podio::Frame frame;

  edm4hep::MCParticleCollection mcParticleCollection;

  for (const auto& particle : particles) {
    auto p = mcParticleCollection->create();
    EDM4hepUtil::writeParticle(particle, p);
  }

  frame.put(std::move(mcParticleCollection), m_cfg.outputParticles);

  std::lock_guard guard(m_writeMutex);
  m_writer.writeFrame(frame, "events");

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
