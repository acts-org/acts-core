// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvParticleWriter.hpp"

#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Io/Csv/CsvInputOutput.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include <Acts/Definitions/Units.hpp>

#include <stdexcept>
#include <vector>

#include "CsvOutputData.hpp"

ActsExamples::CsvParticleWriter::CsvParticleWriter(
    const ActsExamples::CsvParticleWriter::Config& cfg,
    Acts::Logging::Level lvl)
    : WriterT(cfg.inputParticles, "CsvParticleWriter", lvl), m_cfg(cfg) {
  // inputParticles is already checked by base constructor
  if (m_cfg.outputStem.empty()) {
    throw std::invalid_argument("Missing output filename stem");
  }
}

ActsExamples::ProcessCode ActsExamples::CsvParticleWriter::writeT(
    const ActsExamples::AlgorithmContext& ctx,
    const SimParticleContainer& particles) {
  auto pathParticles = perEventFilepath(
      m_cfg.outputDir, m_cfg.outputStem + ".csv", ctx.eventNumber);
  ActsExamples::NamedTupleCsvWriter<ParticleData> writer(pathParticles,
                                                         m_cfg.outputPrecision);

  ParticleData data;
  for (const auto& particle : particles) {
    data.particle_id = particle.particleId().value();
    data.particle_type = particle.pdg();
    data.process = static_cast<decltype(data.process)>(particle.process());
    data.vx = particle.position().x() / Acts::UnitConstants::mm;
    data.vy = particle.position().y() / Acts::UnitConstants::mm;
    data.vz = particle.position().z() / Acts::UnitConstants::mm;
    data.vt = particle.time() / Acts::UnitConstants::mm;
    const auto p = particle.absoluteMomentum() / Acts::UnitConstants::GeV;
    data.px = p * particle.direction().x();
    data.py = p * particle.direction().y();
    data.pz = p * particle.direction().z();
    data.m = particle.mass() / Acts::UnitConstants::GeV;
    data.q = particle.charge() / Acts::UnitConstants::e;
    writer.append(data);
  }

  return ProcessCode::SUCCESS;
}
