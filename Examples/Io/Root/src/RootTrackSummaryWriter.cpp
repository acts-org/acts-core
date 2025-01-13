// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootTrackSummaryWriter.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/MultiTrajectoryHelpers.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/TrackFitting/GsfOptions.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/MultiIndex.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/detail/periodic.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/TruthMatching.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>
#include <ostream>
#include <stdexcept>

#include <TFile.h>
#include <TTree.h>

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;
using Acts::VectorHelpers::theta;

namespace ActsExamples {

RootTrackSummaryWriter::RootTrackSummaryWriter(
    const RootTrackSummaryWriter::Config& config, Acts::Logging::Level level)
    : WriterT(config.inputTracks, "RootTrackSummaryWriter", level),
      m_cfg(config) {
  // tracks collection name is already checked by base ctor
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing output filename");
  }
  if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }

  m_inputParticles.maybeInitialize(m_cfg.inputParticles);
  m_inputTrackParticleMatching.maybeInitialize(
      m_cfg.inputTrackParticleMatching);

  // Setup ROOT I/O
  auto path = m_cfg.filePath;
  m_outputFile = TFile::Open(path.c_str(), m_cfg.fileMode.c_str());
  if (m_outputFile == nullptr) {
    throw std::ios_base::failure("Could not open '" + path + "'");
  }
  m_outputFile->cd();
  m_outputTree = new TTree(m_cfg.treeName.c_str(), m_cfg.treeName.c_str());
  if (m_outputTree == nullptr) {
    throw std::bad_alloc();
  }

  // I/O parameters
  m_outputTree->Branch("event_nr", &m_eventNr);
  m_outputTree->Branch("track_nr", &m_trackNr);

  m_outputTree->Branch("nStates", &m_nStates);
  m_outputTree->Branch("nMeasurements", &m_nMeasurements);
  m_outputTree->Branch("nOutliers", &m_nOutliers);
  m_outputTree->Branch("nHoles", &m_nHoles);
  m_outputTree->Branch("nSharedHits", &m_nSharedHits);
  m_outputTree->Branch("chi2Sum", &m_chi2Sum);
  m_outputTree->Branch("NDF", &m_NDF);
  m_outputTree->Branch("measurementChi2", &m_measurementChi2);
  m_outputTree->Branch("outlierChi2", &m_outlierChi2);
  m_outputTree->Branch("measurementVolume", &m_measurementVolume);
  m_outputTree->Branch("measurementLayer", &m_measurementLayer);
  m_outputTree->Branch("outlierVolume", &m_outlierVolume);
  m_outputTree->Branch("outlierLayer", &m_outlierLayer);

  m_outputTree->Branch("nMajorityHits", &m_nMajorityHits);
  m_outputTree->Branch("majorityParticleId", &m_majorityParticleId);
  m_outputTree->Branch("trackClassification", &m_trackClassification);
  m_outputTree->Branch("t_charge", &m_t_charge);
  m_outputTree->Branch("t_time", &m_t_time);
  m_outputTree->Branch("t_vx", &m_t_vx);
  m_outputTree->Branch("t_vy", &m_t_vy);
  m_outputTree->Branch("t_vz", &m_t_vz);
  m_outputTree->Branch("t_px", &m_t_px);
  m_outputTree->Branch("t_py", &m_t_py);
  m_outputTree->Branch("t_pz", &m_t_pz);
  m_outputTree->Branch("t_theta", &m_t_theta);
  m_outputTree->Branch("t_phi", &m_t_phi);
  m_outputTree->Branch("t_eta", &m_t_eta);
  m_outputTree->Branch("t_p", &m_t_p);
  m_outputTree->Branch("t_pT", &m_t_pT);
  m_outputTree->Branch("t_d0", &m_t_d0);
  m_outputTree->Branch("t_z0", &m_t_z0);
  m_outputTree->Branch("t_prodR", &m_t_prodR);

  m_outputTree->Branch("hasFittedParams", &m_hasFittedParams);
  m_outputTree->Branch("eLOC0_fit", &m_eLOC0_fit);
  m_outputTree->Branch("eLOC1_fit", &m_eLOC1_fit);
  m_outputTree->Branch("ePHI_fit", &m_ePHI_fit);
  m_outputTree->Branch("eTHETA_fit", &m_eTHETA_fit);
  m_outputTree->Branch("eQOP_fit", &m_eQOP_fit);
  m_outputTree->Branch("eT_fit", &m_eT_fit);
  m_outputTree->Branch("err_eLOC0_fit", &m_err_eLOC0_fit);
  m_outputTree->Branch("err_eLOC1_fit", &m_err_eLOC1_fit);
  m_outputTree->Branch("err_ePHI_fit", &m_err_ePHI_fit);
  m_outputTree->Branch("err_eTHETA_fit", &m_err_eTHETA_fit);
  m_outputTree->Branch("err_eQOP_fit", &m_err_eQOP_fit);
  m_outputTree->Branch("err_eT_fit", &m_err_eT_fit);
  m_outputTree->Branch("res_eLOC0_fit", &m_res_eLOC0_fit);
  m_outputTree->Branch("res_eLOC1_fit", &m_res_eLOC1_fit);
  m_outputTree->Branch("res_ePHI_fit", &m_res_ePHI_fit);
  m_outputTree->Branch("res_eTHETA_fit", &m_res_eTHETA_fit);
  m_outputTree->Branch("res_eQOP_fit", &m_res_eQOP_fit);
  m_outputTree->Branch("res_eT_fit", &m_res_eT_fit);
  m_outputTree->Branch("pull_eLOC0_fit", &m_pull_eLOC0_fit);
  m_outputTree->Branch("pull_eLOC1_fit", &m_pull_eLOC1_fit);
  m_outputTree->Branch("pull_ePHI_fit", &m_pull_ePHI_fit);
  m_outputTree->Branch("pull_eTHETA_fit", &m_pull_eTHETA_fit);
  m_outputTree->Branch("pull_eQOP_fit", &m_pull_eQOP_fit);
  m_outputTree->Branch("pull_eT_fit", &m_pull_eT_fit);

  if (m_cfg.writeGsfSpecific) {
    m_outputTree->Branch("max_material_fwd", &m_gsf_max_material_fwd);
    m_outputTree->Branch("sum_material_fwd", &m_gsf_sum_material_fwd);
  }

  if (m_cfg.writeCovMat) {
    // create one branch for every entry of covariance matrix
    // one block for every row of the matrix, every entry gets own branch
    m_outputTree->Branch("cov_eLOC0_eLOC0", &m_cov_eLOC0_eLOC0);
    m_outputTree->Branch("cov_eLOC0_eLOC1", &m_cov_eLOC0_eLOC1);
    m_outputTree->Branch("cov_eLOC0_ePHI", &m_cov_eLOC0_ePHI);
    m_outputTree->Branch("cov_eLOC0_eTHETA", &m_cov_eLOC0_eTHETA);
    m_outputTree->Branch("cov_eLOC0_eQOP", &m_cov_eLOC0_eQOP);
    m_outputTree->Branch("cov_eLOC0_eT", &m_cov_eLOC0_eT);

    m_outputTree->Branch("cov_eLOC1_eLOC0", &m_cov_eLOC1_eLOC0);
    m_outputTree->Branch("cov_eLOC1_eLOC1", &m_cov_eLOC1_eLOC1);
    m_outputTree->Branch("cov_eLOC1_ePHI", &m_cov_eLOC1_ePHI);
    m_outputTree->Branch("cov_eLOC1_eTHETA", &m_cov_eLOC1_eTHETA);
    m_outputTree->Branch("cov_eLOC1_eQOP", &m_cov_eLOC1_eQOP);
    m_outputTree->Branch("cov_eLOC1_eT", &m_cov_eLOC1_eT);

    m_outputTree->Branch("cov_ePHI_eLOC0", &m_cov_ePHI_eLOC0);
    m_outputTree->Branch("cov_ePHI_eLOC1", &m_cov_ePHI_eLOC1);
    m_outputTree->Branch("cov_ePHI_ePHI", &m_cov_ePHI_ePHI);
    m_outputTree->Branch("cov_ePHI_eTHETA", &m_cov_ePHI_eTHETA);
    m_outputTree->Branch("cov_ePHI_eQOP", &m_cov_ePHI_eQOP);
    m_outputTree->Branch("cov_ePHI_eT", &m_cov_ePHI_eT);

    m_outputTree->Branch("cov_eTHETA_eLOC0", &m_cov_eTHETA_eLOC0);
    m_outputTree->Branch("cov_eTHETA_eLOC1", &m_cov_eTHETA_eLOC1);
    m_outputTree->Branch("cov_eTHETA_ePHI", &m_cov_eTHETA_ePHI);
    m_outputTree->Branch("cov_eTHETA_eTHETA", &m_cov_eTHETA_eTHETA);
    m_outputTree->Branch("cov_eTHETA_eQOP", &m_cov_eTHETA_eQOP);
    m_outputTree->Branch("cov_eTHETA_eT", &m_cov_eTHETA_eT);

    m_outputTree->Branch("cov_eQOP_eLOC0", &m_cov_eQOP_eLOC0);
    m_outputTree->Branch("cov_eQOP_eLOC1", &m_cov_eQOP_eLOC1);
    m_outputTree->Branch("cov_eQOP_ePHI", &m_cov_eQOP_ePHI);
    m_outputTree->Branch("cov_eQOP_eTHETA", &m_cov_eQOP_eTHETA);
    m_outputTree->Branch("cov_eQOP_eQOP", &m_cov_eQOP_eQOP);
    m_outputTree->Branch("cov_eQOP_eT", &m_cov_eQOP_eT);

    m_outputTree->Branch("cov_eT_eLOC0", &m_cov_eT_eLOC0);
    m_outputTree->Branch("cov_eT_eLOC1", &m_cov_eT_eLOC1);
    m_outputTree->Branch("cov_eT_ePHI", &m_cov_eT_ePHI);
    m_outputTree->Branch("cov_eT_eTHETA", &m_cov_eT_eTHETA);
    m_outputTree->Branch("cov_eT_eQOP", &m_cov_eT_eQOP);
    m_outputTree->Branch("cov_eT_eT", &m_cov_eT_eT);
  }

  if (m_cfg.writeGx2fSpecific) {
    m_outputTree->Branch("nUpdatesGx2f", &m_nUpdatesGx2f);
  }
}

RootTrackSummaryWriter::~RootTrackSummaryWriter() {
  m_outputFile->Close();
}

ProcessCode RootTrackSummaryWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  if (m_cfg.writeCovMat) {
    ACTS_INFO("Wrote full covariance matrix to tree");
  }
  ACTS_INFO("Wrote parameters of tracks to tree '" << m_cfg.treeName << "' in '"
                                                   << m_cfg.filePath << "'");

  return ProcessCode::SUCCESS;
}

ProcessCode RootTrackSummaryWriter::writeT(const AlgorithmContext& ctx,
                                           const ConstTrackContainer& tracks) {
  // In case we do not have truth info, we bind to a empty collection
  const static SimParticleContainer emptyParticles;
  const static TrackParticleMatching emptyTrackParticleMatching;

  const auto& particles =
      m_inputParticles.isInitialized() ? m_inputParticles(ctx) : emptyParticles;
  const auto& trackParticleMatching =
      m_inputTrackParticleMatching.isInitialized()
          ? m_inputTrackParticleMatching(ctx)
          : emptyTrackParticleMatching;

  // For each particle within a track, how many hits did it contribute
  std::vector<ParticleHitCount> particleHitCounts;

  // Exclusive access to the tree while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  // Get the event number
  m_eventNr = ctx.eventNumber;

  for (const auto& track : tracks) {
    m_trackNr.push_back(track.index());

    // Collect the trajectory summary info
    m_nStates.push_back(track.nTrackStates());
    m_nMeasurements.push_back(track.nMeasurements());
    m_nOutliers.push_back(track.nOutliers());
    m_nHoles.push_back(track.nHoles());
    m_nSharedHits.push_back(track.nSharedHits());
    m_chi2Sum.push_back(track.chi2());
    m_NDF.push_back(track.nDoF());

    {
      std::vector<double> measurementChi2;
      std::vector<std::uint32_t> measurementVolume;
      std::vector<std::uint32_t> measurementLayer;
      std::vector<double> outlierChi2;
      std::vector<std::uint32_t> outlierVolume;
      std::vector<std::uint32_t> outlierLayer;
      for (const auto& state : track.trackStatesReversed()) {
        const auto& geoID = state.referenceSurface().geometryId();
        const auto& volume = geoID.volume();
        const auto& layer = geoID.layer();
        if (state.typeFlags().test(Acts::TrackStateFlag::OutlierFlag)) {
          outlierChi2.push_back(state.chi2());
          outlierVolume.push_back(volume);
          outlierLayer.push_back(layer);
        } else if (state.typeFlags().test(
                       Acts::TrackStateFlag::MeasurementFlag)) {
          measurementChi2.push_back(state.chi2());
          measurementVolume.push_back(volume);
          measurementLayer.push_back(layer);
        }
      }
      m_measurementChi2.push_back(std::move(measurementChi2));
      m_measurementVolume.push_back(std::move(measurementVolume));
      m_measurementLayer.push_back(std::move(measurementLayer));
      m_outlierChi2.push_back(std::move(outlierChi2));
      m_outlierVolume.push_back(std::move(outlierVolume));
      m_outlierLayer.push_back(std::move(outlierLayer));
    }

    // Initialize the truth particle info
    ActsFatras::Barcode majorityParticleId(
        std::numeric_limits<std::size_t>::max());
    TrackMatchClassification trackClassification =
        TrackMatchClassification::Unknown;
    unsigned int nMajorityHits = std::numeric_limits<unsigned int>::max();
    int t_charge = std::numeric_limits<int>::max();
    float t_time = NaNfloat;
    float t_vx = NaNfloat;
    float t_vy = NaNfloat;
    float t_vz = NaNfloat;
    float t_px = NaNfloat;
    float t_py = NaNfloat;
    float t_pz = NaNfloat;
    float t_theta = NaNfloat;
    float t_phi = NaNfloat;
    float t_eta = NaNfloat;
    float t_p = NaNfloat;
    float t_pT = NaNfloat;
    float t_d0 = NaNfloat;
    float t_z0 = NaNfloat;
    float t_qop = NaNfloat;
    float t_prodR = NaNfloat;

    // Get the perigee surface
    const Acts::Surface* pSurface =
        track.hasReferenceSurface() ? &track.referenceSurface() : nullptr;

    // Get the majority truth particle to this track
    auto match = trackParticleMatching.find(track.index());
    bool foundMajorityParticle = false;
    // Get the truth particle info
    if (match != trackParticleMatching.end() &&
        match->second.particle.has_value()) {
      // Get the barcode of the majority truth particle
      majorityParticleId = match->second.particle.value();
      trackClassification = match->second.classification;
      nMajorityHits = match->second.contributingParticles.front().hitCount;

      // Find the truth particle via the barcode
      auto ip = particles.find(majorityParticleId);
      if (ip != particles.end()) {
        foundMajorityParticle = true;

        const auto& particle = *ip;
        ACTS_VERBOSE("Find the truth particle with barcode "
                     << majorityParticleId << "="
                     << majorityParticleId.value());
        // Get the truth particle info at vertex
        t_p = particle.absoluteMomentum();
        t_charge = static_cast<int>(particle.charge());
        t_time = particle.time();
        t_vx = particle.position().x();
        t_vy = particle.position().y();
        t_vz = particle.position().z();
        t_px = t_p * particle.direction().x();
        t_py = t_p * particle.direction().y();
        t_pz = t_p * particle.direction().z();
        t_theta = theta(particle.direction());
        t_phi = phi(particle.direction());
        t_eta = eta(particle.direction());
        t_pT = t_p * perp(particle.direction());
        t_qop = particle.qOverP();
        t_prodR = std::sqrt(t_vx * t_vx + t_vy * t_vy);

        if (pSurface != nullptr) {
          auto intersection =
              pSurface
                  ->intersect(ctx.geoContext, particle.position(),
                              particle.direction(),
                              Acts::BoundaryTolerance::Infinite())
                  .closest();
          auto position = intersection.position();

          // get the truth perigee parameter
          auto lpResult = pSurface->globalToLocal(ctx.geoContext, position,
                                                  particle.direction());
          if (lpResult.ok()) {
            t_d0 = lpResult.value()[Acts::BoundIndices::eBoundLoc0];
            t_z0 = lpResult.value()[Acts::BoundIndices::eBoundLoc1];
          } else {
            ACTS_ERROR("Global to local transformation did not succeed.");
          }
        }
      } else {
        ACTS_DEBUG("Truth particle with barcode "
                   << majorityParticleId << "=" << majorityParticleId.value()
                   << " not found in the input collection!");
      }
    }
    if (!foundMajorityParticle) {
      ACTS_DEBUG("Truth particle for track " << track.tipIndex()
                                             << " not found!");
    }

    // Push the corresponding truth particle info for the track.
    // Always push back even if majority particle not found
    m_majorityParticleId.push_back(majorityParticleId.value());
    m_trackClassification.push_back(static_cast<int>(trackClassification));
    m_nMajorityHits.push_back(nMajorityHits);
    m_t_charge.push_back(t_charge);
    m_t_time.push_back(t_time);
    m_t_vx.push_back(t_vx);
    m_t_vy.push_back(t_vy);
    m_t_vz.push_back(t_vz);
    m_t_px.push_back(t_px);
    m_t_py.push_back(t_py);
    m_t_pz.push_back(t_pz);
    m_t_theta.push_back(t_theta);
    m_t_phi.push_back(t_phi);
    m_t_eta.push_back(t_eta);
    m_t_p.push_back(t_p);
    m_t_pT.push_back(t_pT);
    m_t_d0.push_back(t_d0);
    m_t_z0.push_back(t_z0);
    m_t_prodR.push_back(t_prodR);

    // Initialize the fitted track parameters info
    std::array<float, Acts::eBoundSize> param = {NaNfloat, NaNfloat, NaNfloat,
                                                 NaNfloat, NaNfloat, NaNfloat};
    std::array<float, Acts::eBoundSize> error = {NaNfloat, NaNfloat, NaNfloat,
                                                 NaNfloat, NaNfloat, NaNfloat};

    // get entries of covariance matrix. If no entry, return NaN
    auto getCov = [&](auto i, auto j) { return track.covariance()(i, j); };

    bool hasFittedParams = track.hasReferenceSurface();
    if (hasFittedParams) {
      const auto& parameter = track.parameters();
      for (unsigned int i = 0; i < Acts::eBoundSize; ++i) {
        param[i] = parameter[i];
      }

      const auto& covariance = track.covariance();
      for (unsigned int i = 0; i < Acts::eBoundSize; ++i) {
        error[i] = std::sqrt(covariance(i, i));
      }
    }

    std::array<float, Acts::eBoundSize> res = {NaNfloat, NaNfloat, NaNfloat,
                                               NaNfloat, NaNfloat, NaNfloat};
    std::array<float, Acts::eBoundSize> pull = {NaNfloat, NaNfloat, NaNfloat,
                                                NaNfloat, NaNfloat, NaNfloat};
    if (foundMajorityParticle && hasFittedParams) {
      res = {param[Acts::eBoundLoc0] - t_d0,
             param[Acts::eBoundLoc1] - t_z0,
             Acts::detail::difference_periodic(
                 param[Acts::eBoundPhi], t_phi,
                 static_cast<float>(2 * std::numbers::pi)),
             param[Acts::eBoundTheta] - t_theta,
             param[Acts::eBoundQOverP] - t_qop,
             param[Acts::eBoundTime] - t_time};

      for (unsigned int i = 0; i < Acts::eBoundSize; ++i) {
        pull[i] = res[i] / error[i];
      }
    }

    // Push the fitted track parameters.
    // Always push back even if no fitted track parameters
    m_eLOC0_fit.push_back(param[Acts::eBoundLoc0]);
    m_eLOC1_fit.push_back(param[Acts::eBoundLoc1]);
    m_ePHI_fit.push_back(param[Acts::eBoundPhi]);
    m_eTHETA_fit.push_back(param[Acts::eBoundTheta]);
    m_eQOP_fit.push_back(param[Acts::eBoundQOverP]);
    m_eT_fit.push_back(param[Acts::eBoundTime]);

    m_res_eLOC0_fit.push_back(res[Acts::eBoundLoc0]);
    m_res_eLOC1_fit.push_back(res[Acts::eBoundLoc1]);
    m_res_ePHI_fit.push_back(res[Acts::eBoundPhi]);
    m_res_eTHETA_fit.push_back(res[Acts::eBoundTheta]);
    m_res_eQOP_fit.push_back(res[Acts::eBoundQOverP]);
    m_res_eT_fit.push_back(res[Acts::eBoundTime]);

    m_err_eLOC0_fit.push_back(error[Acts::eBoundLoc0]);
    m_err_eLOC1_fit.push_back(error[Acts::eBoundLoc1]);
    m_err_ePHI_fit.push_back(error[Acts::eBoundPhi]);
    m_err_eTHETA_fit.push_back(error[Acts::eBoundTheta]);
    m_err_eQOP_fit.push_back(error[Acts::eBoundQOverP]);
    m_err_eT_fit.push_back(error[Acts::eBoundTime]);

    m_pull_eLOC0_fit.push_back(pull[Acts::eBoundLoc0]);
    m_pull_eLOC1_fit.push_back(pull[Acts::eBoundLoc1]);
    m_pull_ePHI_fit.push_back(pull[Acts::eBoundPhi]);
    m_pull_eTHETA_fit.push_back(pull[Acts::eBoundTheta]);
    m_pull_eQOP_fit.push_back(pull[Acts::eBoundQOverP]);
    m_pull_eT_fit.push_back(pull[Acts::eBoundTime]);

    m_hasFittedParams.push_back(hasFittedParams);

    if (m_cfg.writeGsfSpecific) {
      using namespace Acts::GsfConstants;
      if (tracks.hasColumn(Acts::hashString(kFwdMaxMaterialXOverX0))) {
        m_gsf_max_material_fwd.push_back(
            track.template component<double>(kFwdMaxMaterialXOverX0));
      } else {
        m_gsf_max_material_fwd.push_back(NaNfloat);
      }

      if (tracks.hasColumn(Acts::hashString(kFwdSumMaterialXOverX0))) {
        m_gsf_sum_material_fwd.push_back(
            track.template component<double>(kFwdSumMaterialXOverX0));
      } else {
        m_gsf_sum_material_fwd.push_back(NaNfloat);
      }
    }

    if (m_cfg.writeCovMat) {
      // write all entries of covariance matrix to output file
      // one branch for every entry of the matrix.
      m_cov_eLOC0_eLOC0.push_back(getCov(0, 0));
      m_cov_eLOC0_eLOC1.push_back(getCov(0, 1));
      m_cov_eLOC0_ePHI.push_back(getCov(0, 2));
      m_cov_eLOC0_eTHETA.push_back(getCov(0, 3));
      m_cov_eLOC0_eQOP.push_back(getCov(0, 4));
      m_cov_eLOC0_eT.push_back(getCov(0, 5));

      m_cov_eLOC1_eLOC0.push_back(getCov(1, 0));
      m_cov_eLOC1_eLOC1.push_back(getCov(1, 1));
      m_cov_eLOC1_ePHI.push_back(getCov(1, 2));
      m_cov_eLOC1_eTHETA.push_back(getCov(1, 3));
      m_cov_eLOC1_eQOP.push_back(getCov(1, 4));
      m_cov_eLOC1_eT.push_back(getCov(1, 5));

      m_cov_ePHI_eLOC0.push_back(getCov(2, 0));
      m_cov_ePHI_eLOC1.push_back(getCov(2, 1));
      m_cov_ePHI_ePHI.push_back(getCov(2, 2));
      m_cov_ePHI_eTHETA.push_back(getCov(2, 3));
      m_cov_ePHI_eQOP.push_back(getCov(2, 4));
      m_cov_ePHI_eT.push_back(getCov(2, 5));

      m_cov_eTHETA_eLOC0.push_back(getCov(3, 0));
      m_cov_eTHETA_eLOC1.push_back(getCov(3, 1));
      m_cov_eTHETA_ePHI.push_back(getCov(3, 2));
      m_cov_eTHETA_eTHETA.push_back(getCov(3, 3));
      m_cov_eTHETA_eQOP.push_back(getCov(3, 4));
      m_cov_eTHETA_eT.push_back(getCov(3, 5));

      m_cov_eQOP_eLOC0.push_back(getCov(4, 0));
      m_cov_eQOP_eLOC1.push_back(getCov(4, 1));
      m_cov_eQOP_ePHI.push_back(getCov(4, 2));
      m_cov_eQOP_eTHETA.push_back(getCov(4, 3));
      m_cov_eQOP_eQOP.push_back(getCov(4, 4));
      m_cov_eQOP_eT.push_back(getCov(4, 5));

      m_cov_eT_eLOC0.push_back(getCov(5, 0));
      m_cov_eT_eLOC1.push_back(getCov(5, 1));
      m_cov_eT_ePHI.push_back(getCov(5, 2));
      m_cov_eT_eTHETA.push_back(getCov(5, 3));
      m_cov_eT_eQOP.push_back(getCov(5, 4));
      m_cov_eT_eT.push_back(getCov(5, 5));
    }

    if (m_cfg.writeGx2fSpecific) {
      if (tracks.hasColumn(Acts::hashString("Gx2fnUpdateColumn"))) {
        int nUpdate = static_cast<int>(
            track.template component<std::uint32_t,
                                     Acts::hashString("Gx2fnUpdateColumn")>());
        m_nUpdatesGx2f.push_back(nUpdate);
      } else {
        m_nUpdatesGx2f.push_back(-1);
      }
    }
  }

  // fill the variables
  m_outputTree->Fill();

  m_trackNr.clear();
  m_nStates.clear();
  m_nMeasurements.clear();
  m_nOutliers.clear();
  m_nHoles.clear();
  m_nSharedHits.clear();
  m_chi2Sum.clear();
  m_NDF.clear();
  m_measurementChi2.clear();
  m_outlierChi2.clear();
  m_measurementVolume.clear();
  m_measurementLayer.clear();
  m_outlierVolume.clear();
  m_outlierLayer.clear();

  m_nMajorityHits.clear();
  m_majorityParticleId.clear();
  m_trackClassification.clear();
  m_t_charge.clear();
  m_t_time.clear();
  m_t_vx.clear();
  m_t_vy.clear();
  m_t_vz.clear();
  m_t_px.clear();
  m_t_py.clear();
  m_t_pz.clear();
  m_t_theta.clear();
  m_t_phi.clear();
  m_t_p.clear();
  m_t_pT.clear();
  m_t_eta.clear();
  m_t_d0.clear();
  m_t_z0.clear();
  m_t_prodR.clear();

  m_hasFittedParams.clear();
  m_eLOC0_fit.clear();
  m_eLOC1_fit.clear();
  m_ePHI_fit.clear();
  m_eTHETA_fit.clear();
  m_eQOP_fit.clear();
  m_eT_fit.clear();
  m_err_eLOC0_fit.clear();
  m_err_eLOC1_fit.clear();
  m_err_ePHI_fit.clear();
  m_err_eTHETA_fit.clear();
  m_err_eQOP_fit.clear();
  m_err_eT_fit.clear();
  m_res_eLOC0_fit.clear();
  m_res_eLOC1_fit.clear();
  m_res_ePHI_fit.clear();
  m_res_eTHETA_fit.clear();
  m_res_eQOP_fit.clear();
  m_res_eT_fit.clear();
  m_pull_eLOC0_fit.clear();
  m_pull_eLOC1_fit.clear();
  m_pull_ePHI_fit.clear();
  m_pull_eTHETA_fit.clear();
  m_pull_eQOP_fit.clear();
  m_pull_eT_fit.clear();

  m_gsf_max_material_fwd.clear();
  m_gsf_sum_material_fwd.clear();

  if (m_cfg.writeCovMat) {
    m_cov_eLOC0_eLOC0.clear();
    m_cov_eLOC0_eLOC1.clear();
    m_cov_eLOC0_ePHI.clear();
    m_cov_eLOC0_eTHETA.clear();
    m_cov_eLOC0_eQOP.clear();
    m_cov_eLOC0_eT.clear();

    m_cov_eLOC1_eLOC0.clear();
    m_cov_eLOC1_eLOC1.clear();
    m_cov_eLOC1_ePHI.clear();
    m_cov_eLOC1_eTHETA.clear();
    m_cov_eLOC1_eQOP.clear();
    m_cov_eLOC1_eT.clear();

    m_cov_ePHI_eLOC0.clear();
    m_cov_ePHI_eLOC1.clear();
    m_cov_ePHI_ePHI.clear();
    m_cov_ePHI_eTHETA.clear();
    m_cov_ePHI_eQOP.clear();
    m_cov_ePHI_eT.clear();

    m_cov_eTHETA_eLOC0.clear();
    m_cov_eTHETA_eLOC1.clear();
    m_cov_eTHETA_ePHI.clear();
    m_cov_eTHETA_eTHETA.clear();
    m_cov_eTHETA_eQOP.clear();
    m_cov_eTHETA_eT.clear();

    m_cov_eQOP_eLOC0.clear();
    m_cov_eQOP_eLOC1.clear();
    m_cov_eQOP_ePHI.clear();
    m_cov_eQOP_eTHETA.clear();
    m_cov_eQOP_eQOP.clear();
    m_cov_eQOP_eT.clear();

    m_cov_eT_eLOC0.clear();
    m_cov_eT_eLOC1.clear();
    m_cov_eT_ePHI.clear();
    m_cov_eT_eTHETA.clear();
    m_cov_eT_eQOP.clear();
    m_cov_eT_eT.clear();
  }

  m_nUpdatesGx2f.clear();

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
