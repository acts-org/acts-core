// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/Csv/CsvSpacePointWriter.hpp"

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

ActsExamples::CsvSpacePointWriter::CsvSpacePointWriter(
    const ActsExamples::CsvSpacePointWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputSpacepoints, "CsvSpacePointWriter", level),
      m_cfg(config) {}

ActsExamples::CsvSpacePointWriter::~CsvSpacePointWriter() = default;

ActsExamples::ProcessCode ActsExamples::CsvSpacePointWriter::finalize() {
  // Write the tree
  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::CsvSpacePointWriter::writeT(
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
