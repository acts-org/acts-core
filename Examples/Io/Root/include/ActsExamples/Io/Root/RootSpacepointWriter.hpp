// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <cstdint>
#include <limits>
#include <mutex>
#include <string>

class TFile;
class TTree;

namespace ActsExamples {
struct AlgorithmContext;

/// Write out space points as a flat TTree.
///
/// Each entry in the TTree corresponds to one space point for optimum writing
/// speed. The event number is part of the written data.
///
/// Safe to use from multiple writer threads. To avoid thread-saftey issues,
/// the writer must be the sole owner of the underlying file. Thus, the
/// output file pointer can not be given from the outside.
class RootSpacepointWriter final : public WriterT<SimSpacePointContainer> {
 public:
  struct Config {
    /// Input particle collection to write.
    std::string inputSpacepoints;
    /// Path to the output file.
    std::string filePath;
    /// Output file access mode.
    std::string fileMode = "RECREATE";
    /// Name of the tree within the output file.
    std::string treeName = "spacepoints";
  };

  /// Construct the particle writer.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  RootSpacepointWriter(const Config& config, Acts::Logging::Level level);

  /// Ensure underlying file is closed.
  ~RootSpacepointWriter() final;

  /// End-of-run hook
  ProcessCode finalize() final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// Type-specific write implementation.
  ///
  /// @param[in] ctx is the algorithm context
  /// @param[in] spacepoints are the spacepoints to be written
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const SimSpacePointContainer& spacepoints) final;

 private:
  Config m_cfg;
  std::mutex m_writeMutex;
  TFile* m_outputFile = nullptr;
  TTree* m_outputTree = nullptr;
  /// Event identifier.
  std::uint32_t m_eventId = 0;
  /// Hit surface identifier.
  std::uint64_t m_measurementId = 0;
  /// Space point surface identifier.
  std::uint64_t m_geometryId = 0;
  /// Global space point position components in mm.
  float m_x = std::numeric_limits<float>::infinity();
  float m_y = std::numeric_limits<float>::infinity();
  float m_z = std::numeric_limits<float>::infinity();
  // Global space point position uncertainties
  float m_var_r = std::numeric_limits<float>::infinity();
  float m_var_z = std::numeric_limits<float>::infinity();
};

}  // namespace ActsExamples
