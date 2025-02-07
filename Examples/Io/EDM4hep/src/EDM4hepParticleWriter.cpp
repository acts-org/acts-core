// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/EDM4hep/EDM4hepParticleWriter.hpp"

#include "ActsExamples/Io/EDM4hep/EDM4hepUtil.hpp"

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
