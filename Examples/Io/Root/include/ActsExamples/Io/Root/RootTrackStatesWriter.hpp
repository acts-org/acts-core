// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/TruthMatching.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

class TFile;
class TTree;
namespace ActsFatras {
class Barcode;
}  // namespace ActsFatras

namespace ActsExamples {
struct AlgorithmContext;

/// @class RootTrackStatesWriter
///
/// Write out tracks (i.e. a vector of trackState at the moment) into a TTree
///
/// Safe to use from multiple writer threads - uses a std::mutex lock.
///
/// Each entry in the TTree corresponds to one track for optimum writing speed.
/// The event number is part of the written data.
///
/// A common file can be provided for the writer to attach his TTree, this is
/// done by setting the Config::rootFile pointer to an existing file.
///
/// Safe to use from multiple writer threads - uses a std::mutex lock.
class RootTrackStatesWriter final : public WriterT<ConstTrackContainer> {
 public:
  struct Config {
    /// Input (fitted) tracks collection
    std::string inputTracks;
    /// Input particles collection.
    std::string inputParticles;
    /// Input track-particle matching.
    std::string inputTrackParticleMatching;
    /// Input collection of simulated hits.
    std::string inputSimHits;
    /// Input collection to map measured hits to simulated hits.
    std::string inputMeasurementSimHitsMap;
    /// output filename.
    std::string filePath = "trackstates.root";
    /// name of the output tree.
    std::string treeName = "trackstates";
    /// file access mode.
    std::string fileMode = "RECREATE";
  };

  /// Constructor
  ///
  /// @param config Configuration struct
  /// @param level Message level declaration
  RootTrackStatesWriter(const Config& config, Acts::Logging::Level level);

  ~RootTrackStatesWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// @brief Write method called by the base class
  /// @param [in] ctx is the algorithm context for event information
  /// @param [in] tracks are what to be written out
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const ConstTrackContainer& tracks) override;

 private:
  enum ParameterType { ePredicted = 0, eFiltered, eSmoothed, eUnbiased, eSize };
  enum class StateType : int {
    eMeasurement = 0,
    eOutlier,
    eHole,
    eMaterial,
    eUnknown,
    eSizeState
  };

  static StateType getStateType(ConstTrackStateProxy state);

  /// The config class
  Config m_cfg;

  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  ReadDataHandle<TrackParticleMatching> m_inputTrackParticleMatching{
      this, "InputTrackParticleMatching"};
  ReadDataHandle<SimHitContainer> m_inputSimHits{this, "InputSimHits"};
  ReadDataHandle<HitSimHitsMap> m_inputMeasurementSimHitsMap{
      this, "InputMeasurementSimHitsMap"};

  /// Mutex used to protect multi-threaded writes
  std::mutex m_writeMutex;

  /// The output file
  TFile* m_outputFile{nullptr};
  /// The output tree
  TTree* m_outputTree{nullptr};

  /// the event number
  std::uint32_t m_eventNr{0};
  /// the track number
  std::uint32_t m_trackNr{0};

  /// number of all states
  unsigned int m_nStates{0};
  /// number of states with measurements
  unsigned int m_nMeasurements{0};

  /// volume identifier
  std::vector<int> m_volumeID;
  /// layer identifier
  std::vector<int> m_layerID;
  /// surface identifier
  std::vector<int> m_moduleID;

  /// track state type
  std::vector<int> m_stateType;

  /// chisq from filtering
  std::vector<float> m_chi2;

  /// path length
  std::vector<float> m_pathLength;

  /// Global truth hit position x
  std::vector<float> m_t_x;
  /// Global truth hit position y
  std::vector<float> m_t_y;
  /// Global truth hit position z
  std::vector<float> m_t_z;
  /// Global truth hit position r
  std::vector<float> m_t_r;
  /// Truth particle direction x at global hit position
  std::vector<float> m_t_dx;
  /// Truth particle direction y at global hit position
  std::vector<float> m_t_dy;
  /// Truth particle direction z at global hit position
  std::vector<float> m_t_dz;

  /// truth parameter eBoundLoc0
  std::vector<float> m_t_eLOC0;
  /// truth parameter eBoundLoc1
  std::vector<float> m_t_eLOC1;
  /// truth parameter ePHI
  std::vector<float> m_t_ePHI;
  /// truth parameter eTHETA
  std::vector<float> m_t_eTHETA;
  /// truth parameter eQOP
  std::vector<float> m_t_eQOP;
  /// truth parameter eT
  std::vector<float> m_t_eT;

  /// event-unique particle identifier a.k.a barcode for hits per each surface
  std::vector<std::vector<std::uint64_t>> m_particleId;

  /// dimension of measurement
  std::vector<int> m_dim_hit;
  /// uncalibrated measurement local x
  std::vector<float> m_lx_hit;
  /// uncalibrated measurement local y
  std::vector<float> m_ly_hit;
  /// uncalibrated measurement global x
  std::vector<float> m_x_hit;
  /// uncalibrated measurement global y
  std::vector<float> m_y_hit;
  /// uncalibrated measurement global z
  std::vector<float> m_z_hit;
  /// hit residual x
  std::vector<float> m_res_x_hit;
  /// hit residual y
  std::vector<float> m_res_y_hit;
  /// hit err x
  std::vector<float> m_err_x_hit;
  /// hit err y
  std::vector<float> m_err_y_hit;
  /// hit pull x
  std::vector<float> m_pull_x_hit;
  /// hit pull y
  std::vector<float> m_pull_y_hit;

  /// number of states which have filtered/predicted/smoothed/unbiased
  /// parameters
  std::array<int, eSize> m_nParams{};
  /// status of the filtered/predicted/smoothed/unbiased parameters
  std::array<std::vector<bool>, eSize> m_hasParams;
  /// predicted/filtered/smoothed/unbiased parameter eLOC0
  std::array<std::vector<float>, eSize> m_eLOC0;
  /// predicted/filtered/smoothed/unbiased parameter eLOC1
  std::array<std::vector<float>, eSize> m_eLOC1;
  /// predicted/filtered/smoothed/unbiased parameter ePHI
  std::array<std::vector<float>, eSize> m_ePHI;
  /// predicted/filtered/smoothed/unbiased parameter eTHETA
  std::array<std::vector<float>, eSize> m_eTHETA;
  /// predicted/filtered/smoothed/unbiased parameter eQOP
  std::array<std::vector<float>, eSize> m_eQOP;
  /// predicted/filtered/smoothed/unbiased parameter eT
  std::array<std::vector<float>, eSize> m_eT;
  /// predicted/filtered/smoothed/unbiased parameter eLOC0 residual
  std::array<std::vector<float>, eSize> m_res_eLOC0;
  /// predicted/filtered/smoothed/unbiased parameter eLOC1 residual
  std::array<std::vector<float>, eSize> m_res_eLOC1;
  /// predicted/filtered/smoothed/unbiased parameter ePHI residual
  std::array<std::vector<float>, eSize> m_res_ePHI;
  /// predicted/filtered/smoothed/unbiased parameter eTHETA residual
  std::array<std::vector<float>, eSize> m_res_eTHETA;
  /// predicted/filtered/smoothed/unbiased parameter eQOP residual
  std::array<std::vector<float>, eSize> m_res_eQOP;
  /// predicted/filtered/smoothed/unbiased parameter eT residual
  std::array<std::vector<float>, eSize> m_res_eT;
  /// predicted/filtered/smoothed/unbiased parameter eLOC0 error
  std::array<std::vector<float>, eSize> m_err_eLOC0;
  /// predicted/filtered/smoothed/unbiased parameter eLOC1 error
  std::array<std::vector<float>, eSize> m_err_eLOC1;
  /// predicted/filtered/smoothed/unbiased parameter ePHI error
  std::array<std::vector<float>, eSize> m_err_ePHI;
  /// predicted/filtered/smoothed/unbiased parameter eTHETA error
  std::array<std::vector<float>, eSize> m_err_eTHETA;
  /// predicted/filtered/smoothed/unbiased parameter eQOP error
  std::array<std::vector<float>, eSize> m_err_eQOP;
  /// predicted/filtered/smoothed/unbiased parameter eT error
  std::array<std::vector<float>, eSize> m_err_eT;
  /// predicted/filtered/smoothed/unbiased parameter eLOC0 pull
  std::array<std::vector<float>, eSize> m_pull_eLOC0;
  /// predicted/filtered/smoothed/unbiased parameter eLOC1 pull
  std::array<std::vector<float>, eSize> m_pull_eLOC1;
  /// predicted/filtered/smoothed/unbiased parameter ePHI pull
  std::array<std::vector<float>, eSize> m_pull_ePHI;
  /// predicted/filtered/smoothed/unbiased parameter eTHETA pull
  std::array<std::vector<float>, eSize> m_pull_eTHETA;
  /// predicted/filtered/smoothed/unbiased parameter eQOP pull
  std::array<std::vector<float>, eSize> m_pull_eQOP;
  /// predicted/filtered/smoothed/unbiased parameter eT pull
  std::array<std::vector<float>, eSize> m_pull_eT;
  /// predicted/filtered/smoothed/unbiased parameter global x
  std::array<std::vector<float>, eSize> m_x;
  /// predicted/filtered/smoothed/unbiased parameter global y
  std::array<std::vector<float>, eSize> m_y;
  /// predicted/filtered/smoothed/unbiased parameter global z
  std::array<std::vector<float>, eSize> m_z;
  /// predicted/filtered/smoothed/unbiased parameter px
  std::array<std::vector<float>, eSize> m_px;
  /// predicted/filtered/smoothed/unbiased parameter py
  std::array<std::vector<float>, eSize> m_py;
  /// predicted/filtered/smoothed/unbiased parameter pz
  std::array<std::vector<float>, eSize> m_pz;
  /// predicted/filtered/smoothed/unbiased parameter eta
  std::array<std::vector<float>, eSize> m_eta;
  /// predicted/filtered/smoothed/unbiased parameter pT
  std::array<std::vector<float>, eSize> m_pT;
};

}  // namespace ActsExamples
