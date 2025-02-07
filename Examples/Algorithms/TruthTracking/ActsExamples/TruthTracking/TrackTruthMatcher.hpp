// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/TruthMatching.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <string>

namespace ActsExamples {

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
  ReadDataHandle<MeasurementParticlesMap> m_inputMeasurementParticlesMap{
      this, "InputMeasurementParticlesMap"};
  WriteDataHandle<TrackParticleMatching> m_outputTrackParticleMatching{
      this, "OutputTrackParticleMatching"};
  WriteDataHandle<ParticleTrackMatching> m_outputParticleTrackMatching{
      this, "OutputParticleTrackMatching"};
};

}  // namespace ActsExamples
