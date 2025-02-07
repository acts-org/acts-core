// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/Obj/ObjPropagationStepsWriter.hpp"

namespace ActsExamples {

ObjPropagationStepsWriter::ObjPropagationStepsWriter(const Config& cfg,
                                                     Acts::Logging::Level level)
    : WriterT<PropagationSummaries>(cfg.collection, "ObjPropagationStepsWriter",
                                    level),
      m_cfg(cfg) {
  if (m_cfg.collection.empty()) {
    throw std::invalid_argument("Missing input collection");
  }
}

/// This implementation holds the actual writing method
/// and is called by the WriterT<>::write interface
ProcessCode ObjPropagationStepsWriter::writeT(
    const AlgorithmContext& context, const PropagationSummaries& summaries) {
  // open per-event file
  std::string path = ActsExamples::perEventFilepath(
      m_cfg.outputDir, "propagation-steps.obj", context.eventNumber);
  std::ofstream os(path, std::ofstream::out | std::ofstream::trunc);
  if (!os) {
    throw std::ios_base::failure("Could not open '" + path + "' to write");
  }

  // Initialize the vertex counter
  unsigned int vCounter = 0;

  for (const auto& summary : summaries) {
    const auto& steps = summary.steps;
    // At least three points to draw
    if (steps.size() > 2) {
      // We start from one
      ++vCounter;
      for (auto& step : steps) {
        // Write the space point
        os << "v " << m_cfg.outputScalor * step.position.x() << " "
           << m_cfg.outputScalor * step.position.y() << " "
           << m_cfg.outputScalor * step.position.z() << '\n';
      }
      // Write out the line - only if we have at least two points created
      std::size_t vBreak = vCounter + steps.size() - 1;
      for (; vCounter < vBreak; ++vCounter) {
        os << "l " << vCounter << " " << vCounter + 1 << '\n';
      }
    }
  }
  return ActsExamples::ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
