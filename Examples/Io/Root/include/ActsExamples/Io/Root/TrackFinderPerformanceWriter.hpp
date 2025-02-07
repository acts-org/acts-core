// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/TruthMatching.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsExamples/Validation/DuplicationPlotTool.hpp"
#include "ActsExamples/Validation/EffPlotTool.hpp"
#include "ActsExamples/Validation/FakeRatePlotTool.hpp"
#include "ActsExamples/Validation/TrackSummaryPlotTool.hpp"

#include <cstddef>
#include <mutex>
#include <string>

class TFile;
class TTree;
namespace ActsFatras {
class Barcode;
}  // namespace ActsFatras

namespace ActsExamples {
struct AlgorithmContext;

/// Write out the performance of CombinatorialKalmanFilter (CKF), e.g. track
/// efficiency, fake rate etc.
///
/// @TODO: add duplication plots
///
/// A common file can be provided for the writer to attach his TTree, this is
/// done by setting the Config::rootFile pointer to an existing file.
///
/// Safe to use from multiple writer threads - uses a std::mutex lock.
class TrackFinderPerformanceWriter final : public WriterT<ConstTrackContainer> {
 public:
  struct Config {
    /// Input (found) tracks collection.
    std::string inputTracks;
    /// Input particles collection.
    std::string inputParticles;
    /// Input track-particle matching.
    std::string inputTrackParticleMatching;
    /// Input track-particle matching.
    std::string inputParticleTrackMatching;
    /// Output filename.
    std::string filePath = "performance_ckf.root";
    /// Output filemode
    std::string fileMode = "RECREATE";

    /// Plot tool configurations.
    EffPlotTool::Config effPlotToolConfig;
    FakeRatePlotTool::Config fakeRatePlotToolConfig;
    DuplicationPlotTool::Config duplicationPlotToolConfig;
    TrackSummaryPlotTool::Config trackSummaryPlotToolConfig;

    /// Additional tracksummary plot tool configs for detector regions
    /// Allows e.g. to do pixel/strip only plots based on a list of volumes
    std::map<std::string, std::set<int>> subDetectorTrackSummaryVolumes;

    /// Write additional matching details to a TTree
    bool writeMatchingDetails = false;
  };

  /// Construct from configuration and log level.
  TrackFinderPerformanceWriter(Config cfg, Acts::Logging::Level lvl);
  ~TrackFinderPerformanceWriter() override;

  /// Finalize plots.
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const ConstTrackContainer& tracks) override;

  Config m_cfg;
  /// Mutex used to protect multi-threaded writes.
  std::mutex m_writeMutex;
  TFile* m_outputFile{nullptr};
  /// Plot tool for efficiency
  EffPlotTool m_effPlotTool;
  EffPlotTool::EffPlotCache m_effPlotCache;
  /// Plot tool for fake rate
  FakeRatePlotTool m_fakeRatePlotTool;
  FakeRatePlotTool::FakeRatePlotCache m_fakeRatePlotCache{};
  /// Plot tool for duplication rate
  DuplicationPlotTool m_duplicationPlotTool;
  DuplicationPlotTool::DuplicationPlotCache m_duplicationPlotCache{};
  /// Plot tool for track hit info
  TrackSummaryPlotTool m_trackSummaryPlotTool;
  TrackSummaryPlotTool::TrackSummaryPlotCache m_trackSummaryPlotCache{};
  std::map<std::string, TrackSummaryPlotTool::TrackSummaryPlotCache>
      m_subDetectorSummaryCaches;

  /// For optional output of the matching details
  TTree* m_matchingTree{nullptr};

  /// Variables to fill in the TTree
  std::uint32_t m_treeEventNr{};
  std::uint64_t m_treeParticleId{};
  bool m_treeIsMatched{};

  // Adding numbers for efficiency, fake, duplicate calculations
  std::size_t m_nTotalTracks = 0;
  std::size_t m_nTotalMatchedTracks = 0;
  std::size_t m_nTotalFakeTracks = 0;
  std::size_t m_nTotalDuplicateTracks = 0;
  std::size_t m_nTotalParticles = 0;
  std::size_t m_nTotalMatchedParticles = 0;
  std::size_t m_nTotalDuplicateParticles = 0;
  std::size_t m_nTotalFakeParticles = 0;

  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  ReadDataHandle<TrackParticleMatching> m_inputTrackParticleMatching{
      this, "InputTrackParticleMatching"};
  ReadDataHandle<ParticleTrackMatching> m_inputParticleTrackMatching{
      this, "InputParticleTrackMatching"};
};

}  // namespace ActsExamples
