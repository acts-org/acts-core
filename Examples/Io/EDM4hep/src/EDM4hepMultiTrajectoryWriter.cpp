// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/EDM4hep/EDM4hepMultiTrajectoryWriter.hpp"

#include "ActsExamples/Io/EDM4hep/EDM4hepUtil.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"

#include <stdexcept>

#include <edm4hep/TrackCollection.h>
#include <podio/Frame.h>

namespace ActsExamples {

EDM4hepMultiTrajectoryWriter::EDM4hepMultiTrajectoryWriter(
    const EDM4hepMultiTrajectoryWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT<TrajectoriesContainer>(config.inputTrajectories,
                                     "EDM4hepMultiTrajectoryWriter", level),
      m_cfg(config),
      m_writer(config.outputPath) {
  if (m_cfg.inputTrajectories.empty()) {
    throw std::invalid_argument("Missing input trajectories collection");
  }

  if (m_cfg.inputMeasurementParticlesMap.empty()) {
    throw std::invalid_argument{"Missing input hit to particle map"};
  }

  m_inputMeasurementParticlesMap.initialize(m_cfg.inputMeasurementParticlesMap);
}

ActsExamples::ProcessCode EDM4hepMultiTrajectoryWriter::finalize() {
  m_writer.finish();

  return ProcessCode::SUCCESS;
}

ProcessCode EDM4hepMultiTrajectoryWriter::writeT(
    const AlgorithmContext& context,
    const TrajectoriesContainer& trajectories) {
  podio::Frame frame{};

  const auto& hitParticlesMap = m_inputMeasurementParticlesMap(context);

  edm4hep::TrackCollection trackCollection;

  for (const auto& from : trajectories) {
    for (const auto& trackTip : from.tips()) {
      auto to = trackCollection.create();
      EDM4hepUtil::writeTrajectory(context.geoContext, m_cfg.Bz, from, to,
                                   trackTip, m_cfg.particleHypothesis,
                                   hitParticlesMap);
    }
  }

  frame.put(std::move(trackCollection), "ActsTracks");

  std::lock_guard guard(m_writeMutex);
  m_writer.writeFrame(frame, "events");

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
