// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include <Acts/Propagator/MaterialInteractor.hpp>
#include <Acts/Utilities/Logger.hpp>

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
  struct Config {
    /// material collection to write
    std::string inputMaterialTracks = "material-tracks";
    /// path of the output file
    std::string filePath = "";
    /// file access mode
    std::string fileMode = "RECREATE";
    /// name of the output tree
    std::string treeName = "material-tracks";

    /// Re-calculate total values from individual steps (for cross-checks)
    bool recalculateTotals = false;
    /// Write aut pre and post step (for G4), otherwise central step position
    bool prePostStep = false;
    /// Write the surface to which the material step correpond
    bool storeSurface = false;
    /// Write the volume to which the material step correpond
    bool storeVolume = false;
    /// Collapse consecutive interactions of a single surface into a single
    /// interaction
    bool collapseInteractions = false;
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

  /// Event identifier.
  std::uint32_t m_eventId = 0;

  /// start global x
  float m_v_x = 0;
  /// start global y
  float m_v_y = 0;
  /// start global z
  float m_v_z = 0;
  /// start global momentum x
  float m_v_px = 0;
  /// start global momentum y
  float m_v_py = 0;
  /// start global momentum z
  float m_v_pz = 0;
  /// start phi direction
  float m_v_phi = 0;
  /// start eta direction
  float m_v_eta = 0;
  /// thickness in X0/L0
  float m_tX0 = 0;
  /// thickness in X0/L0
  float m_tL0 = 0;

  /// step x (start) position (optional)
  std::vector<float> m_step_sx;
  /// step y (start) position (optional)
  std::vector<float> m_step_sy;
  /// step z (start) position (optional)
  std::vector<float> m_step_sz;
  /// step x position
  std::vector<float> m_step_x;
  /// step y position
  std::vector<float> m_step_y;
  /// step z position
  std::vector<float> m_step_z;
  /// step r position
  std::vector<float> m_step_r;
  /// step x (end) position (optional)
  std::vector<float> m_step_ex;
  /// step y (end) position (optional)
  std::vector<float> m_step_ey;
  /// step z (end) position (optional)
  std::vector<float> m_step_ez;
  /// step x direction
  std::vector<float> m_step_dx;
  /// step y direction
  std::vector<float> m_step_dy;
  /// step z direction
  std::vector<float> m_step_dz;
  /// step length
  std::vector<float> m_step_length;
  /// step material x0
  std::vector<float> m_step_X0;
  /// step material l0
  std::vector<float> m_step_L0;
  /// step material A
  std::vector<float> m_step_A;
  /// step material Z
  std::vector<float> m_step_Z;
  /// step material rho
  std::vector<float> m_step_rho;

  /// ID of the surface associated with the step
  std::vector<std::uint64_t> m_sur_id;
  /// Type of the surface associated with the step
  std::vector<std::int32_t> m_sur_type;
  /// x position of the surface intersection associated with the step
  std::vector<float> m_sur_x;
  /// y position of the surface intersection associated with the step
  std::vector<float> m_sur_y;
  /// z position of the surface intersection associated with the step
  std::vector<float> m_sur_z;
  /// r of the position of the surface intersection associated with the step
  std::vector<float> m_sur_r;
  /// the distance to the surface associated with the step
  std::vector<float> m_sur_distance;
  /// path correction when associating material to the given surface
  std::vector<float> m_sur_pathCorrection;
  /// Min range of the surface associated with the step
  std::vector<float> m_sur_range_min;
  /// Max range of the surface associated with the step
  std::vector<float> m_sur_range_max;

  /// ID of the volume associated with the step
  std::vector<std::uint64_t> m_vol_id;
};

}  // namespace ActsExamples
