// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/EDM4hep/EDM4hepTrackWriter.hpp"

#include "Acts/Plugins/EDM4hep/EDM4hepUtil.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepUtil.hpp"

#include <stdexcept>

#include <edm4hep/TrackCollection.h>
#include <podio/Frame.h>

namespace ActsExamples {

EDM4hepTrackWriter::EDM4hepTrackWriter(const Config& config,
                                       Acts::Logging::Level level)
    : WriterT<ConstTrackContainer>(config.inputTracks, "EDM4hepTrackWriter",
                                   level),
      m_cfg(config),
      m_writer(config.outputPath) {
  if (m_cfg.inputTracks.empty()) {
    throw std::invalid_argument("Missing input trajectories collection");
  }
}

ActsExamples::ProcessCode EDM4hepTrackWriter::finalize() {
  m_writer.finish();

  return ProcessCode::SUCCESS;
}

ProcessCode EDM4hepTrackWriter::writeT(const AlgorithmContext& context,
                                       const ConstTrackContainer& tracks) {
  podio::Frame frame{};

  edm4hep::TrackCollection trackCollection;

  for (const auto& from : tracks) {
    auto to = trackCollection.create();
    Acts::EDM4hepUtil::writeTrack(context.geoContext, from, to, m_cfg.Bz);
  }

  frame.put(std::move(trackCollection), m_cfg.outputTracks);

  std::lock_guard guard(m_writeMutex);
  m_writer.writeFrame(frame, "events");

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
