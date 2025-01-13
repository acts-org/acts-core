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
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/TruthMatching.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include <TMatrixD.h>

class TFile;
class TTree;

namespace ActsExamples {

/// @class RootTrackSummaryWriter
///
/// Write out the information (including number of measurements, outliers, holes
/// etc., fitted track parameters and corresponding majority truth particle
/// info) of the reconstructed tracks into a TTree.
///
/// Safe to use from multiple writer threads - uses a std::mutex lock.
///
/// Each entry in the TTree corresponds to all reconstructed tracks in one
/// single event. The event number is part of the written data.
///
/// A common file can be provided for the writer to attach his TTree, this is
/// done by setting the Config::rootFile pointer to an existing file.
///
/// Safe to use from multiple writer threads - uses a std::mutex lock.
class RootTrackSummaryWriter final : public WriterT<ConstTrackContainer> {
 public:
  struct Config {
    /// Input (fitted) tracks collection
    std::string inputTracks;
    /// Input particles collection (optional).
    std::string inputParticles;
    /// Input track-particle matching (optional).
    std::string inputTrackParticleMatching;
    /// Output filename.
    std::string filePath = "tracksummary.root";
    /// Name of the output tree.
    std::string treeName = "tracksummary";
    /// File access mode.
    std::string fileMode = "RECREATE";
    /// Switch for adding full covariance matrix to output file.
    bool writeCovMat = false;
    /// Write GSF specific things (for now only some material statistics)
    bool writeGsfSpecific = false;
    /// Write GX2F specific things
    bool writeGx2fSpecific = false;
  };

  /// Constructor
  ///
  /// @param config Configuration struct
  /// @param level Message level declaration
  RootTrackSummaryWriter(const Config& config, Acts::Logging::Level level);
  ~RootTrackSummaryWriter() override;

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
  /// The config class
  Config m_cfg;

  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  ReadDataHandle<TrackParticleMatching> m_inputTrackParticleMatching{
      this, "InputTrackParticleMatching"};

  /// Mutex used to protect multi-threaded writes
  std::mutex m_writeMutex;
  /// The output file
  TFile* m_outputFile{nullptr};
  /// The output tree
  TTree* m_outputTree{nullptr};
  /// The event number
  std::uint32_t m_eventNr{0};
  /// The track number in event
  std::vector<std::uint32_t> m_trackNr;

  /// The number of states
  std::vector<unsigned int> m_nStates;
  /// The number of measurements
  std::vector<unsigned int> m_nMeasurements;
  /// The number of outliers
  std::vector<unsigned int> m_nOutliers;
  /// The number of holes
  std::vector<unsigned int> m_nHoles;
  /// The number of shared hits
  std::vector<unsigned int> m_nSharedHits;
  /// The total chi2
  std::vector<float> m_chi2Sum;
  /// The number of ndf of the measurements+outliers
  std::vector<unsigned int> m_NDF;
  /// The chi2 on all measurement states
  std::vector<std::vector<double>> m_measurementChi2;
  /// The chi2 on all outlier states
  std::vector<std::vector<double>> m_outlierChi2;
  /// The volume id of the measurements
  std::vector<std::vector<std::uint32_t>> m_measurementVolume;
  /// The layer id of the measurements
  std::vector<std::vector<std::uint32_t>> m_measurementLayer;
  /// The volume id of the outliers
  std::vector<std::vector<std::uint32_t>> m_outlierVolume;
  /// The layer id of the outliers
  std::vector<std::vector<std::uint32_t>> m_outlierLayer;

  // The majority truth particle info
  /// The number of hits from majority particle
  std::vector<unsigned int> m_nMajorityHits;
  /// The particle Id of the majority particle
  std::vector<std::uint64_t> m_majorityParticleId;
  /// The classification of the reconstructed track
  std::vector<int> m_trackClassification;
  /// Charge of majority particle
  std::vector<int> m_t_charge;
  /// Time of majority particle
  std::vector<float> m_t_time;
  /// Vertex x positions of majority particle
  std::vector<float> m_t_vx;
  /// Vertex y positions of majority particle
  std::vector<float> m_t_vy;
  /// Vertex z positions of majority particle
  std::vector<float> m_t_vz;
  /// Initial momenta px of majority particle
  std::vector<float> m_t_px;
  /// Initial momenta py of majority particle
  std::vector<float> m_t_py;
  /// Initial momenta pz of majority particle
  std::vector<float> m_t_pz;
  /// Initial momenta theta of majority particle
  std::vector<float> m_t_theta;
  /// Initial momenta phi of majority particle
  std::vector<float> m_t_phi;
  /// Initial abs momenta of majority particle
  std::vector<float> m_t_p;
  /// Initial momenta pT of majority particle
  std::vector<float> m_t_pT;
  /// Initial momenta eta of majority particle
  std::vector<float> m_t_eta;
  /// The extrapolated truth transverse impact parameter
  std::vector<float> m_t_d0;
  /// The extrapolated truth longitudinal impact parameter
  std::vector<float> m_t_z0;
  /// Production radius of majority particle
  std::vector<float> m_t_prodR;

  /// If the track has fitted parameter
  std::vector<bool> m_hasFittedParams;
  // The fitted parameters
  /// Fitted parameters eBoundLoc0 of track
  std::vector<float> m_eLOC0_fit;
  /// Fitted parameters eBoundLoc1 of track
  std::vector<float> m_eLOC1_fit;
  /// Fitted parameters ePHI of track
  std::vector<float> m_ePHI_fit;
  /// Fitted parameters eTHETA of track
  std::vector<float> m_eTHETA_fit;
  /// Fitted parameters eQOP of track
  std::vector<float> m_eQOP_fit;
  /// Fitted parameters eT of track
  std::vector<float> m_eT_fit;
  // The error of fitted parameters
  /// Fitted parameters eLOC err of track
  std::vector<float> m_err_eLOC0_fit;
  /// Fitted parameters eBoundLoc1 err of track
  std::vector<float> m_err_eLOC1_fit;
  /// Fitted parameters ePHI err of track
  std::vector<float> m_err_ePHI_fit;
  /// Fitted parameters eTHETA err of track
  std::vector<float> m_err_eTHETA_fit;
  /// Fitted parameters eQOP err of track
  std::vector<float> m_err_eQOP_fit;
  /// Fitted parameters eT err of track
  std::vector<float> m_err_eT_fit;
  // The residual of fitted parameters
  /// Fitted parameters eLOC res of track
  std::vector<float> m_res_eLOC0_fit;
  /// Fitted parameters eBoundLoc1 res of track
  std::vector<float> m_res_eLOC1_fit;
  /// Fitted parameters ePHI res of track
  std::vector<float> m_res_ePHI_fit;
  /// Fitted parameters eTHETA res of track
  std::vector<float> m_res_eTHETA_fit;
  /// Fitted parameters eQOP res of track
  std::vector<float> m_res_eQOP_fit;
  /// Fitted parameters eT res of track
  std::vector<float> m_res_eT_fit;
  // The pull of fitted parameters
  /// Fitted parameters eLOC pull of track
  std::vector<float> m_pull_eLOC0_fit;
  /// Fitted parameters eBoundLoc1 pull of track
  std::vector<float> m_pull_eLOC1_fit;
  /// Fitted parameters ePHI pull of track
  std::vector<float> m_pull_ePHI_fit;
  /// Fitted parameters eTHETA pull of track
  std::vector<float> m_pull_eTHETA_fit;
  /// Fitted parameters eQOP pull of track
  std::vector<float> m_pull_eQOP_fit;
  /// Fitted parameters eT pull of track
  std::vector<float> m_pull_eT_fit;

  // entries of the full covariance matrix. One block for every row of the
  // matrix
  std::vector<float> m_cov_eLOC0_eLOC0;
  std::vector<float> m_cov_eLOC0_eLOC1;
  std::vector<float> m_cov_eLOC0_ePHI;
  std::vector<float> m_cov_eLOC0_eTHETA;
  std::vector<float> m_cov_eLOC0_eQOP;
  std::vector<float> m_cov_eLOC0_eT;

  std::vector<float> m_cov_eLOC1_eLOC0;
  std::vector<float> m_cov_eLOC1_eLOC1;
  std::vector<float> m_cov_eLOC1_ePHI;
  std::vector<float> m_cov_eLOC1_eTHETA;
  std::vector<float> m_cov_eLOC1_eQOP;
  std::vector<float> m_cov_eLOC1_eT;

  std::vector<float> m_cov_ePHI_eLOC0;
  std::vector<float> m_cov_ePHI_eLOC1;
  std::vector<float> m_cov_ePHI_ePHI;
  std::vector<float> m_cov_ePHI_eTHETA;
  std::vector<float> m_cov_ePHI_eQOP;
  std::vector<float> m_cov_ePHI_eT;

  std::vector<float> m_cov_eTHETA_eLOC0;
  std::vector<float> m_cov_eTHETA_eLOC1;
  std::vector<float> m_cov_eTHETA_ePHI;
  std::vector<float> m_cov_eTHETA_eTHETA;
  std::vector<float> m_cov_eTHETA_eQOP;
  std::vector<float> m_cov_eTHETA_eT;

  std::vector<float> m_cov_eQOP_eLOC0;
  std::vector<float> m_cov_eQOP_eLOC1;
  std::vector<float> m_cov_eQOP_ePHI;
  std::vector<float> m_cov_eQOP_eTHETA;
  std::vector<float> m_cov_eQOP_eQOP;
  std::vector<float> m_cov_eQOP_eT;

  std::vector<float> m_cov_eT_eLOC0;
  std::vector<float> m_cov_eT_eLOC1;
  std::vector<float> m_cov_eT_ePHI;
  std::vector<float> m_cov_eT_eTHETA;
  std::vector<float> m_cov_eT_eQOP;
  std::vector<float> m_cov_eT_eT;

  std::vector<float> m_gsf_max_material_fwd;
  std::vector<float> m_gsf_sum_material_fwd;

  /// The number of updates (gx2f)
  std::vector<int> m_nUpdatesGx2f;
};

}  // namespace ActsExamples
