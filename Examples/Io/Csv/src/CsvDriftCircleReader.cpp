// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvDriftCircleReader.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/EventData/DriftCircle.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Io/Csv/CsvInputOutput.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Hit.hpp"

#include <array>
#include <stdexcept>

#include "CsvOutputData.hpp"

ActsExamples::CsvDriftCircleReader::CsvDriftCircleReader(
    const ActsExamples::CsvDriftCircleReader::Config& config,
    Acts::Logging::Level level)
    : m_cfg(config),
      // TODO check that all files (hits,cells,truth) exists
      m_eventsRange(
          determineEventFilesRange(m_cfg.inputDir, m_cfg.inputStem + ".csv")),
      m_logger(Acts::getDefaultLogger("CsvDriftCircleReader", level)) {
  if (m_cfg.inputStem.empty()) {
    throw std::invalid_argument("Missing input filename stem");
  }
  if (m_cfg.outputDriftCircles.empty()) {
    throw std::invalid_argument("Missing drift circle output collection");
  }

  m_outputDriftCircles.initialize(m_cfg.outputDriftCircles);
}

std::string ActsExamples::CsvDriftCircleReader::CsvDriftCircleReader::name()
    const {
  return "CsvDriftCircleReader";
}

std::pair<std::size_t, std::size_t>
ActsExamples::CsvDriftCircleReader::availableEvents() const {
  return m_eventsRange;
}

ActsExamples::ProcessCode ActsExamples::CsvDriftCircleReader::read(
    const ActsExamples::AlgorithmContext& ctx) {
  auto path = perEventFilepath(m_cfg.inputDir, m_cfg.inputStem + ".csv",
                               ctx.eventNumber);

  ActsExamples::NamedTupleCsvReader<MuonDriftCircleData> reader(path);

  DriftCircleContainer DriftCircles;
  MuonDriftCircleData data;

  while (reader.read(data)) {
    ActsFatras::Hit::Vector3 tube_pos{
        data.tubePositionx * Acts::UnitConstants::mm,
        data.tubePositiony * Acts::UnitConstants::mm,
        data.tubePositionz * Acts::UnitConstants::mm};

    DriftCircles.push_back(DriftCircle(std::move(tube_pos), data.driftRadius,
                                       0.0f, data.stationName, data.stationEta,
                                       data.stationPhi, data.multilayer,
                                       data.tubelayer, data.tube));
  }

  // write the ordered data to the EventStore (according to geometry_id).
  m_outputDriftCircles(ctx, std::move(DriftCircles));

  return ActsExamples::ProcessCode::SUCCESS;
}
