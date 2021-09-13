// This file is part of the Acts project.
//
// Copyright (C) 2017 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvParticleReader.hpp"

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Utilities/Paths.hpp"

#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

#include <dfe/dfe_io_dsv.hpp>

#include "CsvOutputData.hpp"

ActsExamples::CsvParticleReader::CsvParticleReader(
    const ActsExamples::CsvParticleReader::Config& config,
    Acts::Logging::Level level)
    : m_cfg(config),
      m_eventsRange(
          determineEventFilesRange(m_cfg.inputDir, m_cfg.inputStem + ".csv")),
      m_logger(Acts::getDefaultLogger("CsvParticleReader", level)) {
  if (m_cfg.inputStem.empty()) {
    throw std::invalid_argument("Missing input filename stem");
  }
  if (m_cfg.outputParticles.empty()) {
    throw std::invalid_argument("Missing output collection");
  }
}

std::string ActsExamples::CsvParticleReader::CsvParticleReader::name() const {
  return "CsvParticleReader";
}

std::pair<size_t, size_t> ActsExamples::CsvParticleReader::availableEvents()
    const {
  return m_eventsRange;
}

ActsExamples::ProcessCode ActsExamples::CsvParticleReader::read(
    const ActsExamples::AlgorithmContext& ctx) {
  SimParticleContainer::sequence_type unordered;

  auto path = perEventFilepath(m_cfg.inputDir, m_cfg.inputStem + ".csv",
                               ctx.eventNumber);
  // vt and m are an optional columns
  dfe::NamedTupleCsvReader<ParticleData> reader(path, {"vt", "m"});
  ParticleData data;

  while (reader.read(data)) {
    ActsFatras::Particle particle(ActsFatras::Barcode(data.particle_id),
                                  Acts::PdgParticle(data.particle_type),
                                  data.q * Acts::UnitConstants::e,
                                  data.m * Acts::UnitConstants::GeV);
    particle.setProcess(static_cast<ActsFatras::ProcessType>(data.process));
    particle.setPosition4(
        data.vx * Acts::UnitConstants::mm, data.vy * Acts::UnitConstants::mm,
        data.vz * Acts::UnitConstants::mm, data.vt * Acts::UnitConstants::ns);
    // Only used for direction; normalization/units do not matter
    particle.setDirection(data.px, data.py, data.pz);
    particle.setAbsoluteMomentum(std::hypot(data.px, data.py, data.pz) *
                                 Acts::UnitConstants::GeV);
    unordered.push_back(std::move(particle));
  }

  // Write ordered particles container to the EventStore
  SimParticleContainer particles;
  particles.insert(unordered.begin(), unordered.end());
  ctx.eventStore.add(m_cfg.outputParticles, std::move(particles));

  return ProcessCode::SUCCESS;
}
