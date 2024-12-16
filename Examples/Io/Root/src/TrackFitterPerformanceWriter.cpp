// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/TrackFitterPerformanceWriter.hpp"

#include "Acts/EventData/MultiTrajectoryHelpers.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/MultiIndex.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <TFile.h>

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::phi;

ActsExamples::TrackFitterPerformanceWriter::TrackFitterPerformanceWriter(
    ActsExamples::TrackFitterPerformanceWriter::Config config,
    Acts::Logging::Level level)
    : WriterT(config.inputTracks, "TrackFitterPerformanceWriter", level),
      m_cfg(std::move(config)),
      m_resPlotTool(m_cfg.resPlotToolConfig, level),
      m_effPlotTool(m_cfg.effPlotToolConfig, level),
      m_trackSummaryPlotTool(m_cfg.trackSummaryPlotToolConfig, level) {
  // trajectories collection name is already checked by base ctor
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Missing particles input collection");
  }
  if (m_cfg.inputTrackParticleMatching.empty()) {
    throw std::invalid_argument("Missing input track particles matching");
  }
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing output filename");
  }

  m_inputParticles.initialize(m_cfg.inputParticles);
  m_inputTrackParticleMatching.initialize(m_cfg.inputTrackParticleMatching);

  // the output file can not be given externally since TFile accesses to the
  // same file from multiple threads are unsafe.
  // must always be opened internally
  auto path = m_cfg.filePath;
  m_outputFile = TFile::Open(path.c_str(), "RECREATE");
  if (m_outputFile == nullptr) {
    throw std::invalid_argument("Could not open '" + path + "'");
  }

  // initialize the residual and efficiency plots tool
  m_resPlotTool.book(m_resPlotCache);
  m_effPlotTool.book(m_effPlotCache);
  m_trackSummaryPlotTool.book(m_trackSummaryPlotCache);
}

ActsExamples::TrackFitterPerformanceWriter::~TrackFitterPerformanceWriter() {
  m_resPlotTool.clear(m_resPlotCache);
  m_effPlotTool.clear(m_effPlotCache);
  m_trackSummaryPlotTool.clear(m_trackSummaryPlotCache);

  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ActsExamples::ProcessCode
ActsExamples::TrackFitterPerformanceWriter::finalize() {
  // fill residual and pull details into additional hists
  m_resPlotTool.refinement(m_resPlotCache);

  if (m_outputFile != nullptr) {
    m_outputFile->cd();
    m_resPlotTool.write(m_resPlotCache);
    m_effPlotTool.write(m_effPlotCache);
    m_trackSummaryPlotTool.write(m_trackSummaryPlotCache);

    ACTS_INFO("Wrote performance plots to '" << m_outputFile->GetPath() << "'");
  }
  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::TrackFitterPerformanceWriter::writeT(
    const AlgorithmContext& ctx, const ConstTrackContainer& tracks) {
  // Read truth input collections
  const auto& particles = m_inputParticles(ctx);
  const auto& trackParticleMatching = m_inputTrackParticleMatching(ctx);

  // Truth particles with corresponding reconstructed tracks
  std::vector<ActsFatras::Barcode> reconParticleIds;
  reconParticleIds.reserve(particles.size());
  // For each particle within a track, how many hits did it contribute
  std::vector<ParticleHitCount> particleHitCounts;

  // Exclusive access to the tree while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  // Loop over all tracks
  for (const auto& track : tracks) {
    // Select reco track with fitted parameters
    if (!track.hasReferenceSurface()) {
      ACTS_WARNING("No fitted track parameters.");
      continue;
    }
    Acts::BoundTrackParameters fittedParameters =
        track.createParametersAtReference();

    // Get the truth-matched particle
    auto imatched = trackParticleMatching.find(track.index());
    if (imatched == trackParticleMatching.end()) {
      ACTS_DEBUG("No truth particle associated with this track, index = "
                 << track.index() << " tip index = " << track.tipIndex());
      continue;
    }
    const auto& particleMatch = imatched->second;

    if (!particleMatch.particle.has_value()) {
      ACTS_DEBUG("No truth particle associated with this track.");
      continue;
    }

    // Get the barcode of the majority truth particle
    SimBarcode majorityParticleId = particleMatch.particle.value();

    // Find the truth particle via the barcode
    auto ip = particles.find(majorityParticleId);
    if (ip == particles.end()) {
      ACTS_WARNING("Majority particle not found in the particles collection.");
      continue;
    }

    // Record this majority particle ID of this trajectory
    reconParticleIds.push_back(ip->particleId());
    // Fill the residual plots
    m_resPlotTool.fill(m_resPlotCache, ctx.geoContext, ip->initialState(),
                       fittedParameters);
    // Fill the trajectory summary info
    m_trackSummaryPlotTool.fill(m_trackSummaryPlotCache, fittedParameters,
                                track.nTrackStates(), track.nMeasurements(),
                                track.nOutliers(), track.nHoles(),
                                track.nSharedHits());
  }

  // Fill the efficiency, defined as the ratio between number of tracks with
  // fitted parameter and total truth tracks (assumes one truth partilce has
  // one truth track)
  for (const auto& particle : particles) {
    bool isReconstructed = false;
    // Check if the particle has been reconstructed
    if (rangeContainsValue(reconParticleIds, particle.particleId())) {
      isReconstructed = true;
    }
    // Loop over all the other truth particle and find the distance to the
    // closest one
    double minDeltaR = -1;
    for (const auto& closeParticle : particles) {
      if (closeParticle.particleId() == particle.particleId()) {
        continue;
      }
      double p_phi = phi(particle.direction());
      double p_eta = eta(particle.direction());
      double c_phi = phi(closeParticle.direction());
      double c_eta = eta(closeParticle.direction());
      double distance = sqrt(pow(p_phi - c_phi, 2) + pow(p_eta - c_eta, 2));
      if (minDeltaR == -1 || distance < minDeltaR) {
        minDeltaR = distance;
      }
    }
    m_effPlotTool.fill(m_effPlotCache, particle.initialState(), minDeltaR,
                       isReconstructed);
  }

  return ProcessCode::SUCCESS;
}
