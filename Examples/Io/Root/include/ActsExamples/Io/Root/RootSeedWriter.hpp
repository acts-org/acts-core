// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <cstdint>
#include <mutex>
#include <string>

class TFile;
class TTree;

namespace ActsExamples {
struct AlgorithmContext;

/// Write out seeds as a flat TTree.
///
/// Each entry in the TTree corresponds to one seed for optimum writing
/// speed. The event number is part of the written data.
///
/// Safe to use from multiple writer threads. To avoid thread-saftey issues,
/// the writer must be the sole owner of the underlying file. Thus, the
/// output file pointer can not be given from the outside.
class RootSeedWriter final : public WriterT<SimSeedContainer> {
 public:
  struct Config {
    /// Input particle collection to write.
    std::string inputSeeds;
    /// Path to the output file.
    std::string filePath;
    /// Output file access mode.
    std::string fileMode = "RECREATE";
    /// Name of the tree within the output file.
    std::string treeName = "seeds";
    /// The writing mode
    std::string writingMode = "small";
  };

  /// Construct the seed writer.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  RootSeedWriter(const Config& config, Acts::Logging::Level level);

  /// Ensure underlying file is closed.
  ~RootSeedWriter() final;

  /// End-of-run hook
  ProcessCode finalize() final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// Type-specific write implementation.
  ///
  /// @param[in] ctx is the algorithm context
  /// @param[in] seeds are the seeds to be written
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const SimSeedContainer& seeds) final;

 private:
  Config m_cfg;
  std::mutex m_writeMutex;
  TFile* m_outputFile = nullptr;
  TTree* m_outputTree = nullptr;
  /// Event identifier.
  std::uint32_t m_eventId = 0;
  /// Hit surface identifier.
  std::uint64_t m_measurementId_1 = 0;
  std::uint64_t m_measurementId_2 = 0;
  std::uint64_t m_measurementId_3 = 0;
  /// Space point surface identifier.
  std::uint64_t m_geometryId_1 = 0;
  std::uint64_t m_geometryId_2 = 0;
  std::uint64_t m_geometryId_3 = 0;
  /// Global space point position components in mm. init to NaN
  float m_x_1 = std::numeric_limits<float>::signaling_NaN();
  float m_x_2 = std::numeric_limits<float>::signaling_NaN();
  float m_x_3 = std::numeric_limits<float>::signaling_NaN();
  float m_y_1 = std::numeric_limits<float>::signaling_NaN();
  float m_y_2 = std::numeric_limits<float>::signaling_NaN();
  float m_y_3 = std::numeric_limits<float>::signaling_NaN();
  float m_z_1 = std::numeric_limits<float>::signaling_NaN();
  float m_z_2 = std::numeric_limits<float>::signaling_NaN();
  float m_z_3 = std::numeric_limits<float>::signaling_NaN();
  // Global space point position uncertainties
  float m_var_r_1 = std::numeric_limits<float>::signaling_NaN();
  float m_var_r_2 = std::numeric_limits<float>::signaling_NaN();
  float m_var_r_3 = std::numeric_limits<float>::signaling_NaN();
  float m_var_z_1 = std::numeric_limits<float>::signaling_NaN();
  float m_var_z_2 = std::numeric_limits<float>::signaling_NaN();
  float m_var_z_3 = std::numeric_limits<float>::signaling_NaN();
  // Seed vertex position
  double m_z_vertex = std::numeric_limits<double>::signaling_NaN();
  // Seed quality
  float m_seed_quality = std::numeric_limits<float>::signaling_NaN();
};

}  // namespace ActsExamples
