// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <Acts/Definitions/Algebra.hpp>
#include <Acts/Plugins/Root/RootMaterialTrack.hpp>
#include <Acts/Propagator/MaterialInteractor.hpp>
#include <Acts/Utilities/Logger.hpp>
#include <ActsExamples/Framework/ProcessCode.hpp>
#include <ActsExamples/Framework/WriterT.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class TFile;
class TTree;

namespace ActsExamples {
struct AlgorithmContext;
}  // namespace ActsExamples

namespace Acts {
// Using some short hands for Recorded Material
using RecordedMaterial = MaterialInteractor::result_type;
// And recorded material track
// - this is start:  position, start momentum
//   and the Recorded material
using RecordedMaterialTrack =
    std::pair<std::pair<Acts::Vector3, Acts::Vector3>, RecordedMaterial>;
}  // namespace Acts

namespace ActsExamples {

/// @class RootMaterialTrackWriter
///
/// @brief Writes out MaterialTrack collections from a root file
///
/// This service is the root implementation of the IWriterT.
/// It writes out a MaterialTrack which is usually generated from
/// Geant4 material mapping
class RootMaterialTrackWriter
    : public WriterT<
          std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>> {
 public:
  struct Config : public Acts::RootMaterialTrack::Config {
    /// material collection to write
    std::string inputMaterialTracks = "material_tracks";
    /// path of the output file
    std::string filePath = "";
    /// file access mode
    std::string fileMode = "RECREATE";
    /// name of the output tree
    std::string treeName = "material_tracks";
  };

  /// Constructor with
  /// @param config configuration struct
  /// @param level logging level
  RootMaterialTrackWriter(const Config& config,
                          Acts::Logging::Level level = Acts::Logging::INFO);

  /// Virtual destructor
  ~RootMaterialTrackWriter() override;

  /// Framework initialize method
  ActsExamples::ProcessCode finalize() override;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 protected:
  // This implementation holds the actual writing method
  /// and is called by the WriterT<>::write interface
  ///
  /// @param ctx The Algorithm context with per event information
  /// @param clusters is the data to be written out
  ProcessCode writeT(
      const AlgorithmContext& ctx,
      const std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>&
          materialtracks) override;

 private:
  /// The config class
  Config m_cfg;
  /// mutex used to protect multi-threaded writes
  std::mutex m_writeMutex;
  /// The output file name
  TFile* m_outputFile = nullptr;
  /// The output tree name
  TTree* m_outputTree = nullptr;
  /// The root material track object
  Acts::RootMaterialTrack m_rootMaterialTrack;
};

}  // namespace ActsExamples
