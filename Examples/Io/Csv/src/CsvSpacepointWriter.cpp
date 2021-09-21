// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvSpacepointWriter.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "Acts/Definitions/Units.hpp"

#include "ActsExamples/Utilities/Paths.hpp"

#include <ios>
#include <optional>
#include <stdexcept>

#include <dfe/dfe_io_dsv.hpp>

#include "CsvOutputData.hpp"

ActsExamples::CsvSpacepointWriter::CsvSpacepointWriter(
    const ActsExamples::CsvSpacepointWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputSpacepoints, "CsvSpacepointWriter", level), m_cfg(config) {

}

ActsExamples::CsvSpacepointWriter::~CsvSpacepointWriter() {}

ActsExamples::ProcessCode ActsExamples::CsvSpacepointWriter::endRun() {
  // Write the tree
  return ProcessCode::SUCCESS;
}


ActsExamples::ProcessCode ActsExamples::CsvSpacepointWriter::writeT(
    const AlgorithmContext& ctx, const SimSpacePointContainer& spacepoints) {

  // Open per-event file for all components
  std::string pathSP =
      perEventFilepath(m_cfg.outputDir, "spacepoint.csv", ctx.eventNumber);

  dfe::NamedTupleCsvWriter<SpacepointData> writerSP(pathSP, m_cfg.outputPrecision);

  SpacepointData spData;
  for(const auto& sp: spacepoints) {
    spData.measurement_id = sp.measurementIndex();
    spData.x = sp.x();
    spData.y = sp.y();
    spData.z = sp.z();
    spData.var_r = sp.varianceR();
    spData.var_z = sp.varianceZ();
    writerSP.append(spData);
  }
  return ActsExamples::ProcessCode::SUCCESS;
}