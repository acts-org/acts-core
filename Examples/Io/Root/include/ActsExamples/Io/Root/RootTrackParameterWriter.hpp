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
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <mutex>
#include <string>

class TFile;
class TTree;

namespace ActsExamples {
struct AlgorithmContext;

using TrackParameterWriter = WriterT<TrackParametersContainer>;

/// Write out the track parameters from both simulation and those estimated from
/// reconstructed seeds into a TTree
///
/// Each entry in the TTree corresponds to one seed for optimum writing
/// speed. The event number is part of the written data.
class RootTrackParameterWriter final : public TrackParameterWriter {
 public:
  struct Config {
    /// Input estimated track parameters collection.
    std::string inputTrackParameters;
    /// Input reconstructed proto tracks collection.
    std::string inputProtoTracks;
    /// Input particles collection.
    std::string inputParticles;
    /// Input collection of simulated hits.
    std::string inputSimHits;
    /// Input hit-particles map collection.
    std::string inputMeasurementParticlesMap;
    /// Input collection to map measured hits to simulated hits.
    std::string inputMeasurementSimHitsMap;
    /// output filename.
    std::string filePath = "estimatedparams.root";
    /// name of the output tree.
    std::string treeName = "estimatedparams";
    /// file access mode.
    std::string fileMode = "RECREATE";
  };

  /// Constructor
  ///
  /// @param config Configuration struct
  /// @param level Message level declaration
  RootTrackParameterWriter(const Config& config,
                           Acts::Logging::Level level = Acts::Logging::INFO);

  /// Virtual destructor
  ~RootTrackParameterWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// @brief Write method called by the base class
  /// @param [in] ctx is the algorithm context for event information
  /// @param [in] trackParams are parameters to write
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const TrackParametersContainer& trackParams) override;

 private:
  Config m_cfg;  ///< The config class

  ReadDataHandle<ProtoTrackContainer> m_inputProtoTracks{this,
                                                         "InputProtoTracks"};
  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  ReadDataHandle<SimHitContainer> m_inputSimHits{this, "InputSimHits"};
  ReadDataHandle<HitParticlesMap> m_inputMeasurementParticlesMap{
      this, "InputMeasurementParticlesMap"};
  ReadDataHandle<HitSimHitsMap> m_inputMeasurementSimHitsMap{
      this, "InputMeasurementSimHitsMap"};

  /// Mutex used to protect multi-threaded writes
  std::mutex m_writeMutex;
  TFile* m_outputFile{nullptr};
  TTree* m_outputTree{nullptr};

  int m_eventNr{0};

  int m_volumeId{0};
  int m_layerId{0};
  int m_surfaceId{0};

  // Track parameters
  float m_loc0{NaNfloat};
  float m_loc1{NaNfloat};
  float m_phi{NaNfloat};
  float m_theta{NaNfloat};
  float m_qop{NaNfloat};
  float m_time{NaNfloat};

  float m_err_loc0{NaNfloat};
  float m_err_loc1{NaNfloat};
  float m_err_phi{NaNfloat};
  float m_err_theta{NaNfloat};
  float m_err_qop{NaNfloat};
  float m_err_time{NaNfloat};

  int m_charge{0};
  float m_p{NaNfloat};
  float m_pt{NaNfloat};
  float m_eta{NaNfloat};

  // Truth parameters
  /// Whether the seed is matched with truth
  bool m_t_matched{false};
  std::uint64_t m_t_particleId{0};
  unsigned int m_nMajorityHits{0};

  float m_t_loc0{NaNfloat};
  float m_t_loc1{NaNfloat};
  float m_t_phi{NaNfloat};
  float m_t_theta{NaNfloat};
  float m_t_qop{NaNfloat};
  float m_t_time{NaNfloat};

  int m_t_charge{0};
  float m_t_p{NaNfloat};
  float m_t_pt{NaNfloat};
  float m_t_eta{NaNfloat};

  float m_res_loc0{NaNfloat};
  float m_res_loc1{NaNfloat};
  float m_res_phi{NaNfloat};
  float m_res_theta{NaNfloat};
  float m_res_qop{NaNfloat};
  float m_res_time{NaNfloat};

  float m_pull_loc0{NaNfloat};
  float m_pull_loc1{NaNfloat};
  float m_pull_phi{NaNfloat};
  float m_pull_theta{NaNfloat};
  float m_pull_qop{NaNfloat};
  float m_pull_time{NaNfloat};
};

}  // namespace ActsExamples
