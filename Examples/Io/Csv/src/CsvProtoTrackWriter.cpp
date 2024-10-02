// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvProtoTrackWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Io/Csv/CsvInputOutput.hpp"
#include "ActsExamples/Utilities/EventDataTransforms.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsExamples/Utilities/Range.hpp"

#include <ios>
#include <optional>
#include <stdexcept>

#include "CsvOutputData.hpp"

ActsExamples::CsvProtoTrackWriter::CsvProtoTrackWriter(
    const ActsExamples::CsvProtoTrackWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputPrototracks, "CsvProtoTrackWriter", level),
      m_cfg(config) {
  m_inputSpacepoints.initialize(m_cfg.inputSpacepoints);
}

ActsExamples::CsvProtoTrackWriter::~CsvProtoTrackWriter() = default;

ActsExamples::ProcessCode ActsExamples::CsvProtoTrackWriter::finalize() {
  // Write the tree
  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::CsvProtoTrackWriter::writeT(
    const AlgorithmContext& ctx, const ProtoTrackContainer& tracks) {
  const auto& spacepoints = m_inputSpacepoints(ctx);

  // Open per-event file for all components
  std::string path =
      perEventFilepath(m_cfg.outputDir, "prototracks.csv", ctx.eventNumber);

  ActsExamples::NamedTupleCsvWriter<ProtoTrackData> writer(
      path, m_cfg.outputPrecision);

  for (auto trackId = 0ul; trackId < tracks.size(); ++trackId) {
    for (Index measurmentId : tracks[trackId]) {
      const auto spr = findSpacePointForIndex(measurmentId, spacepoints);
      if (spr == nullptr) {
        ACTS_WARNING("Could not convert index " << measurmentId
                                                << " to spacepoint");
        continue;
      }
      const auto& sp = *spr;
      writer.append({trackId, measurmentId, sp.x(), sp.y(), sp.z()});
    }
  }
  return ActsExamples::ProcessCode::SUCCESS;
}
