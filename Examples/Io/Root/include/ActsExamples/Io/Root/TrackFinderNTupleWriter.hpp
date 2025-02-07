// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/TruthMatching.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <memory>
#include <string>

namespace ActsExamples {

/// Write track finder performance measures.
///
/// Only considers the track finding itself, i.e. grouping of hits into tracks,
/// and computes relevant per-track and per-particles statistics.
class TrackFinderNTupleWriter final : public WriterT<ConstTrackContainer> {
 public:
  struct Config {
    /// Input reconstructed track collection.
    std::string inputTracks;
    /// Input particles collection.
    std::string inputParticles;
    /// Input particle-measurements map collection.
    std::string inputParticleMeasurementsMap;
    /// Input proto track-particle matching.
    std::string inputTrackParticleMatching;
    /// Output filename.
    std::string filePath = "performance_track_finder.root";
    /// Output file mode
    std::string fileMode = "RECREATE";
    /// Output tree name for the tracks
    std::string treeNameTracks = "track_finder_tracks";
    /// Output tree name for the particles
    std::string treeNameParticles = "track_finder_particles";
  };

  /// Constructor
  /// @param config the configuration
  /// @param level The log level
  TrackFinderNTupleWriter(Config config, Acts::Logging::Level level);

  ~TrackFinderNTupleWriter() override;

  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const;

 private:
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const ConstTrackContainer& tracks) override;

  struct Impl;

  std::unique_ptr<Impl> m_impl;
};

}  // namespace ActsExamples
