// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvSpacepointWriter.hpp"

#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsExamples/Io/Csv/CsvInputOutput.hpp"
#include "ActsExamples/Utilities/Paths.hpp"

#include <string>
#include <vector>

#include "CsvOutputData.hpp"

ActsExamples::CsvSpacepointWriter::CsvSpacepointWriter(
    const ActsExamples::CsvSpacepointWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputSpacepoints, "CsvSpacepointWriter", level),
      m_cfg(config) {}

ActsExamples::CsvSpacepointWriter::~CsvSpacepointWriter() = default;

ActsExamples::ProcessCode ActsExamples::CsvSpacepointWriter::finalize() {
  // Write the tree
  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::CsvSpacepointWriter::writeT(
    const AlgorithmContext& ctx, const SimSpacePointContainer& spacepoints) {
  // Open per-event file for all components
  std::string pathSP =
      perEventFilepath(m_cfg.outputDir, "spacepoint.csv", ctx.eventNumber);

  ActsExamples::NamedTupleCsvWriter<SpacepointData> writerSP(
      pathSP, m_cfg.outputPrecision);

  SpacepointData spData{};
  for (const auto& sp : spacepoints) {
    const auto slink = sp.sourceLinks()[0].get<IndexSourceLink>();

    spData.measurement_id = slink.index();
    spData.geometry_id = slink.geometryId().value();
    spData.x = sp.x();
    spData.y = sp.y();
    spData.z = sp.z();
    spData.var_r = sp.varianceR();
    spData.var_z = sp.varianceZ();
    writerSP.append(spData);
  }
  return ActsExamples::ProcessCode::SUCCESS;
}
