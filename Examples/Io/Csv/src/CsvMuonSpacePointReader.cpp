
// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "ActsExamples/Io/Csv/CsvMuonSpacePointReader.hpp"


#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/EventData/MuonSpacePoint.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Io/Csv/CsvInputOutput.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Hit.hpp"

#include <array>
#include <stdexcept>

#include "CsvOutputData.hpp"



namespace ActsExamples{
CsvMuonSpacePointReader::CsvMuonSpacePointReader(const Config& config,
                                                 Acts::Logging::Level level): 
      m_cfg{config},
      // TODO check that all files (hits,cells,truth) exists
      m_eventsRange{determineEventFilesRange(m_cfg.inputDir, m_cfg.inputStem + ".csv")},
      m_logger{Acts::getDefaultLogger("CsvMuonSpacePointReader", level)} {

  if (m_cfg.inputStem.empty()) {
    throw std::invalid_argument("Missing input filename stem");
  }
  if (m_cfg.outputSpacePoints.empty()) {
    throw std::invalid_argument("Missing space point output collection");
  }

  m_outputSpacePoints.initialize(m_cfg.outputSpacePoints);
}

std::string CsvMuonSpacePointReader::CsvMuonSpacePointReader::name() const {
  return "CsvMuonSpacePointReader";
}

std::pair<std::size_t, std::size_t>
CsvMuonSpacePointReader::availableEvents() const {
  return m_eventsRange;
}

ProcessCode CsvMuonSpacePointReader::read(const AlgorithmContext& ctx) {
  auto path = perEventFilepath(m_cfg.inputDir, m_cfg.inputStem + ".csv",
                               ctx.eventNumber);

  NamedTupleCsvReader<MuonSpacePointData> reader(path);

  MuonSpacePointData data;

//   SimHitContainer::sequence_type unordered;
    SpacePointContainer spacePoints{};

  while (reader.read(data)) {
    // unordered.push_back(SimHit(compressId(f), data.pdgId, pos, mom, mom, -1));
  }
//   simHits.insert(unordered.begin(), unordered.end());

  // write the ordered data to the EventStore (according to geometry_id).
  m_outputSpacePoints(ctx, std::move(spacePoints));

  return ProcessCode::SUCCESS;
}

}