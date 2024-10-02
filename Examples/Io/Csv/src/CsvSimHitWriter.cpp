// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvSimHitWriter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Io/Csv/CsvInputOutput.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Hit.hpp"

#include <stdexcept>
#include <vector>

#include "CsvOutputData.hpp"

ActsExamples::CsvSimHitWriter::CsvSimHitWriter(
    const ActsExamples::CsvSimHitWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputSimHits, "CsvSimHitWriter", level), m_cfg(config) {
  // inputSimHits is already checked by base constructor
  if (m_cfg.outputStem.empty()) {
    throw std::invalid_argument("Missing output filename stem");
  }
}

ActsExamples::ProcessCode ActsExamples::CsvSimHitWriter::writeT(
    const AlgorithmContext& ctx, const ActsExamples::SimHitContainer& simHits) {
  // open per-event file for all simhit components
  std::string pathSimHit = perEventFilepath(
      m_cfg.outputDir, m_cfg.outputStem + ".csv", ctx.eventNumber);

  ActsExamples::NamedTupleCsvWriter<SimHitData> writerSimHit(
      pathSimHit, m_cfg.outputPrecision);

  // CsvOutputData struct
  SimHitData simhit;
  // Write data from internal impl. to output-side struct
  for (const auto& simHit : simHits) {
    // local simhit information in global coord.
    const Acts::Vector4& globalPos4 = simHit.fourPosition();
    const Acts::Vector4& momentum4Before = simHit.momentum4Before();

    simhit.geometry_id = simHit.geometryId().value();
    simhit.particle_id = simHit.particleId().value();
    // hit position
    simhit.tx = globalPos4[Acts::ePos0] / Acts::UnitConstants::mm;
    simhit.ty = globalPos4[Acts::ePos1] / Acts::UnitConstants::mm;
    simhit.tz = globalPos4[Acts::ePos2] / Acts::UnitConstants::mm;
    simhit.tt = globalPos4[Acts::eTime] / Acts::UnitConstants::mm;
    // particle four-momentum before interaction
    simhit.tpx = momentum4Before[Acts::eMom0] / Acts::UnitConstants::GeV;
    simhit.tpy = momentum4Before[Acts::eMom1] / Acts::UnitConstants::GeV;
    simhit.tpz = momentum4Before[Acts::eMom2] / Acts::UnitConstants::GeV;
    simhit.te = momentum4Before[Acts::eEnergy] / Acts::UnitConstants::GeV;
    // particle four-momentum change due to interaction
    const auto delta4 = simHit.momentum4After() - momentum4Before;
    simhit.deltapx = delta4[Acts::eMom0] / Acts::UnitConstants::GeV;
    simhit.deltapy = delta4[Acts::eMom1] / Acts::UnitConstants::GeV;
    simhit.deltapz = delta4[Acts::eMom2] / Acts::UnitConstants::GeV;
    simhit.deltae = delta4[Acts::eEnergy] / Acts::UnitConstants::GeV;
    // TODO write hit index along the particle trajectory
    simhit.index = simHit.index();
    writerSimHit.append(simhit);
  }  // end simHit loop

  return ActsExamples::ProcessCode::SUCCESS;
}
