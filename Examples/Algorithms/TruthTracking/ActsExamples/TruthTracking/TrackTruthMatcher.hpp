// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/TruthMatching.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <string>

namespace ActsExamples {

struct AlgorithmContext;

/// Matches tracks to truth particles and vice versa
class TrackTruthMatcher final : public IAlgorithm {
 public:
  struct Config {
    /// Input (fitted) tracks collection
    std::string inputTracks;
    /// Input particles collection.
    std::string inputParticles;
    /// Input hit-particles map collection.
    std::string inputMeasurementParticlesMap;
    /// Output track-particle matching.
    std::string outputTrackParticleMatching;
    /// Output track-particle matching.
    std::string outputParticleTrackMatching;

    /// Matching ratio for track to particle matching
    double matchingRatio = 0.5;
    /// Whether to use double matching (track to particle and particle to track)
    bool doubleMatching = true;
  };

  TrackTruthMatcher(const Config& config, Acts::Logging::Level level);

  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<ConstTrackContainer> m_inputTracks{this, "InputTracks"};
  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  ReadDataHandle<HitParticlesMap> m_inputMeasurementParticlesMap{
      this, "InputMeasurementParticlesMap"};
  WriteDataHandle<TrackParticleMatching> m_outputTrackParticleMatching{
      this, "OutputTrackParticleMatching"};
  WriteDataHandle<ParticleTrackMatching> m_outputParticleTrackMatching{
      this, "OutputParticleTrackMatching"};
};

}  // namespace ActsExamples
