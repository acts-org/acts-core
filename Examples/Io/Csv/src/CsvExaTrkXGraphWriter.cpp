// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/Csv/CsvExaTrkXGraphWriter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Io/Csv/CsvInputOutput.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsFatras/EventData/Barcode.hpp"

#include <stdexcept>
#include <vector>

#include "CsvOutputData.hpp"

ActsExamples::CsvExaTrkXGraphWriter::CsvExaTrkXGraphWriter(
    const ActsExamples::CsvExaTrkXGraphWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputGraph, "CsvExaTrkXGraphWriter", level),
      m_cfg(config) {}

ActsExamples::ProcessCode ActsExamples::CsvExaTrkXGraphWriter::writeT(
    const ActsExamples::AlgorithmContext& ctx, const Graph& graph) {
  assert(graph.weights.empty() ||
         (graph.edges.size() / 2 == graph.weights.size()));
  assert(graph.edges.size() % 2 == 0);

  if (graph.weights.empty()) {
    ACTS_DEBUG("No weights provide, write default value of 1");
  }

  std::string path = perEventFilepath(
      m_cfg.outputDir, m_cfg.outputStem + ".csv", ctx.eventNumber);

  ActsExamples::NamedTupleCsvWriter<GraphData> writer(path);

  const auto nEdges = graph.edges.size() / 2;
  for (auto i = 0ul; i < nEdges; ++i) {
    GraphData edge{};
    edge.edge0 = graph.edges[2 * i];
    edge.edge1 = graph.edges[2 * i + 1];
    edge.weight = graph.weights.empty() ? 1.f : graph.weights[i];
    writer.append(edge);
  }

  return ActsExamples::ProcessCode::SUCCESS;
}
