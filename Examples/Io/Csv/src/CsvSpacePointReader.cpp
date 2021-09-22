// This file is part of the Acts project.
//
// Copyright (C) 2017 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvSpacePointReader.hpp"

#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include <Acts/Definitions/Units.hpp>

#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

#include <dfe/dfe_io_dsv.hpp>

#include "CsvOutputData.hpp"

ActsExamples::CsvSpacePointReader::CsvSpacePointReader(
    const ActsExamples::CsvSpacePointReader::Config& cfg,
    Acts::Logging::Level lvl){
  m_cfg = cfg;
  if (m_cfg.inputStem.empty()) {
    throw std::invalid_argument("Missing input filename stem");
  }
  auto& filename = m_cfg.inputCollection.empty() ? cfg.inputStem :
                   cfg.inputStem+'_'+cfg.inputCollection;
  m_eventsRange = determineEventFilesRange(cfg.inputDir, filename + ".csv");
  m_logger = Acts::getDefaultLogger("CsvSpacePointReader", lvl);
}

std::string ActsExamples::CsvSpacePointReader::CsvSpacePointReader::name() const {
  return "CsvSpacePointReader";
}

std::pair<size_t, size_t> ActsExamples::CsvSpacePointReader::availableEvents()
    const {
  return m_eventsRange;
}

ActsExamples::ProcessCode ActsExamples::CsvSpacePointReader::read(
    const ActsExamples::AlgorithmContext& ctx) {
  SimSpacePointContainer spacePoints;
  
  const auto& filename = m_cfg.inputCollection.empty() ? m_cfg.inputStem :
                   m_cfg.inputStem+'_'+m_cfg.inputCollection;
  const auto& path = perEventFilepath(m_cfg.inputDir, filename + ".csv", ctx.eventNumber);

  dfe::NamedTupleCsvReader<SpacePointData> reader(path);
  SpacePointData data;

  while (reader.read(data)) {
    Acts::Vector3 globalPos(data.sp_x, data.sp_y, data.sp_z);
    
    if (m_cfg.inputCollection=="pixel" || m_cfg.inputCollection=="strip" || m_cfg.inputCollection=="overlap")
      spacePoints.emplace_back(globalPos, data.sp_covr, data.sp_covz, data.measurement_id);
    else {
      ACTS_ERROR("Invalid space point type " << m_cfg.inputStem);
      return ProcessCode::ABORT;
    }
  }


  ACTS_DEBUG("Created " << spacePoints.size() << " " << m_cfg.inputCollection << " space points");
  ctx.eventStore.add("PixelSpacePoints", std::move(spacePoints));

  return ProcessCode::SUCCESS;
}
