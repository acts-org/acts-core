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
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <cstdint>
#include <mutex>
#include <string>

class TFile;
class TTree;

namespace ActsExamples {
struct AlgorithmContext;

/// Write out simulated hits as a flat TTree.
///
/// Each entry in the TTree corresponds to one hit for optimum writing
/// speed. The event number is part of the written data.
///
/// Safe to use from multiple writer threads. To avoid thread-saftey issues,
/// the writer must be the sole owner of the underlying file. Thus, the
/// output file pointer can not be given from the outside.
class RootSimHitWriter final : public WriterT<SimHitContainer> {
 public:
  struct Config {
    /// Input sim hit collection to write.
    std::string inputSimHits;
    /// Path to the output file.
    std::string filePath;
    /// Output file access mode.
    std::string fileMode = "RECREATE";
    /// Name of the tree within the output file.
    std::string treeName = "hits";
  };

  /// Construct the particle writer.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  RootSimHitWriter(const Config& config, Acts::Logging::Level level);

  /// Ensure underlying file is closed.
  ~RootSimHitWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// Type-specific write implementation.
  ///
  /// @param[in] ctx is the algorithm context
  /// @param[in] hits are the hits to be written
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const SimHitContainer& hits) override;

 private:
  Config m_cfg;
  std::mutex m_writeMutex;
  TFile* m_outputFile = nullptr;
  TTree* m_outputTree = nullptr;
  /// Event identifier.
  std::uint32_t m_eventId = 0;
  /// Hit surface identifier.
  std::uint64_t m_geometryId = 0;
  /// Event-unique particle identifier a.k.a. barcode.
  std::uint64_t m_particleId = 0;
  /// True global hit position components in mm.
  float m_tx = 0, m_ty = 0, m_tz = 0;
  // True global hit time in ns.
  float m_tt = 0;
  /// True particle four-momentum in GeV at hit position before interaction.
  float m_tpx = 0, m_tpy = 0, m_tpz = 0, m_te = 0;
  /// True change in particle four-momentum in GeV due to interactions.
  float m_deltapx = 0, m_deltapy = 0, m_deltapz = 0, m_deltae = 0;
  /// Hit index along the particle trajectory
  std::int32_t m_index = 0;
  // Decoded hit surface identifier components.
  std::uint32_t m_volumeId = 0;
  std::uint32_t m_boundaryId = 0;
  std::uint32_t m_layerId = 0;
  std::uint32_t m_approachId = 0;
  std::uint32_t m_sensitiveId = 0;
};

}  // namespace ActsExamples
