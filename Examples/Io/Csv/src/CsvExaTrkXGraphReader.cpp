// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvExaTrkXGraphReader.hpp"

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
#include <ActsExamples/Utilities/Paths.hpp>

#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

#include "CsvOutputData.hpp"

namespace ActsExamples {

CsvExaTrkXGraphReader::CsvExaTrkXGraphReader(const Config& config,
                                             Acts::Logging::Level level)
    : m_cfg(config),
      m_eventsRange(
          determineEventFilesRange(m_cfg.inputDir, m_cfg.inputStem + ".csv")),
      m_logger(Acts::getDefaultLogger("CsvExaTrkXGraphReader", level)) {
  if (m_cfg.inputStem.empty()) {
    throw std::invalid_argument("Missing input filename stem");
  }

  m_outputGraph.initialize(m_cfg.outputGraph);
}

std::pair<std::size_t, std::size_t> CsvExaTrkXGraphReader::availableEvents()
    const {
  return m_eventsRange;
}

ProcessCode CsvExaTrkXGraphReader::read(const AlgorithmContext& ctx) {
  SimParticleContainer::sequence_type unordered;

  auto path = perEventFilepath(m_cfg.inputDir, m_cfg.inputStem + ".csv",
                               ctx.eventNumber);
  // vt and m are an optional columns
  ActsExamples::NamedTupleCsvReader<GraphData> reader(path, {"vt", "m"});
  GraphData data;

  Graph g;

  while (reader.read(data)) {
    g.edges.push_back(data.edge0);
    g.edges.push_back(data.edge1);
    g.weights.push_back(data.weight);
  }

  m_outputGraph(ctx, std::move(g));

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
