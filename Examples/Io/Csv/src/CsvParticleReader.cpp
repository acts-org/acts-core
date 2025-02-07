// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/Csv/CsvParticleReader.hpp"

#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Io/Csv/CsvInputOutput.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/EventData/ProcessType.hpp"

#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

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

  m_outputParticles.initialize(m_cfg.outputParticles);
}

std::string ActsExamples::CsvParticleReader::CsvParticleReader::name() const {
  return "CsvParticleReader";
}

std::pair<std::size_t, std::size_t>
ActsExamples::CsvParticleReader::availableEvents() const {
  return m_eventsRange;
}

ActsExamples::ProcessCode ActsExamples::CsvParticleReader::read(
    const ActsExamples::AlgorithmContext& ctx) {
  SimParticleContainer::sequence_type unordered;

  auto path = perEventFilepath(m_cfg.inputDir, m_cfg.inputStem + ".csv",
                               ctx.eventNumber);
  // vt and m are an optional columns
  ActsExamples::NamedTupleCsvReader<ParticleData> reader(path, {"vt", "m"});
  ParticleData data;

  while (reader.read(data)) {
    SimParticleState particle(ActsFatras::Barcode(data.particle_id),
                              Acts::PdgParticle{data.particle_type},
                              data.q * Acts::UnitConstants::e,
                              data.m * Acts::UnitConstants::GeV);
    particle.setProcess(static_cast<ActsFatras::ProcessType>(data.process));
    particle.setPosition4(
        data.vx * Acts::UnitConstants::mm, data.vy * Acts::UnitConstants::mm,
        data.vz * Acts::UnitConstants::mm, data.vt * Acts::UnitConstants::mm);
    // Only used for direction; normalization/units do not matter
    particle.setDirection(data.px, data.py, data.pz);
    particle.setAbsoluteMomentum(std::hypot(data.px, data.py, data.pz) *
                                 Acts::UnitConstants::GeV);
    unordered.push_back(SimParticle(particle, particle));
  }

  // Write ordered particles container to the EventStore
  SimParticleContainer particles;
  particles.insert(unordered.begin(), unordered.end());
  m_outputParticles(ctx, std::move(particles));

  return ProcessCode::SUCCESS;
}
