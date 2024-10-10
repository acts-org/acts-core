// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootTrackParameterWriter.hpp"

#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Utilities/MultiIndex.hpp"
#include "ActsExamples/EventData/AverageSimHits.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsExamples/Utilities/Range.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Hit.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <cmath>
#include <cstddef>
#include <ios>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#include <TFile.h>
#include <TTree.h>

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;
using Acts::VectorHelpers::theta;

namespace ActsExamples {

RootTrackParameterWriter::RootTrackParameterWriter(
    const RootTrackParameterWriter::Config& config, Acts::Logging::Level level)
    : TrackParameterWriter(config.inputTrackParameters,
                           "RootTrackParameterWriter", level),
      m_cfg(config) {
  if (m_cfg.inputProtoTracks.empty()) {
    throw std::invalid_argument("Missing proto tracks input collection");
  }
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Missing particles input collection");
  }
  if (m_cfg.inputSimHits.empty()) {
    throw std::invalid_argument("Missing simulated hits input collection");
  }
  if (m_cfg.inputMeasurementParticlesMap.empty()) {
    throw std::invalid_argument("Missing hit-particles map input collection");
  }
  if (m_cfg.inputMeasurementSimHitsMap.empty()) {
    throw std::invalid_argument(
        "Missing hit-simulated-hits map input collection");
  }
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing output filename");
  }
  if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }

  m_inputProtoTracks.initialize(m_cfg.inputProtoTracks);
  m_inputParticles.initialize(m_cfg.inputParticles);
  m_inputSimHits.initialize(m_cfg.inputSimHits);
  m_inputMeasurementParticlesMap.initialize(m_cfg.inputMeasurementParticlesMap);
  m_inputMeasurementSimHitsMap.initialize(m_cfg.inputMeasurementSimHitsMap);

  // Setup ROOT I/O
  if (m_outputFile == nullptr) {
    auto path = m_cfg.filePath;
    m_outputFile = TFile::Open(path.c_str(), m_cfg.fileMode.c_str());
    if (m_outputFile == nullptr) {
      throw std::ios_base::failure("Could not open '" + path + "'");
    }
  }
  m_outputFile->cd();
  m_outputTree = new TTree(m_cfg.treeName.c_str(), m_cfg.treeName.c_str());
  if (m_outputTree == nullptr) {
    throw std::bad_alloc();
  }

  m_outputTree->Branch("event_nr", &m_eventNr);

  m_outputTree->Branch("volumeId", &m_volumeId);
  m_outputTree->Branch("layerId", &m_layerId);
  m_outputTree->Branch("surfaceId", &m_surfaceId);

  // The estimated track parameters
  m_outputTree->Branch("loc0", &m_loc0);
  m_outputTree->Branch("loc1", &m_loc1);
  m_outputTree->Branch("phi", &m_phi);
  m_outputTree->Branch("theta", &m_theta);
  m_outputTree->Branch("qop", &m_qop);
  m_outputTree->Branch("time", &m_time);

  // The estimated track parameters errors
  m_outputTree->Branch("err_loc0", &m_err_loc0);
  m_outputTree->Branch("err_loc1", &m_err_loc1);
  m_outputTree->Branch("err_phi", &m_err_phi);
  m_outputTree->Branch("err_theta", &m_err_theta);
  m_outputTree->Branch("err_qop", &m_err_qop);
  m_outputTree->Branch("err_time", &m_err_time);

  // Some derived quantities from the estimated track parameters
  m_outputTree->Branch("charge", &m_charge);
  m_outputTree->Branch("p", &m_p);
  m_outputTree->Branch("pt", &m_pt);
  m_outputTree->Branch("eta", &m_eta);

  // The truth meta
  m_outputTree->Branch("truthMatched", &m_t_matched);
  m_outputTree->Branch("particleId", &m_t_particleId);
  m_outputTree->Branch("nMajorityHits", &m_nMajorityHits);

  // The truth track parameters
  m_outputTree->Branch("particleId", &m_t_particleId);
  m_outputTree->Branch("t_loc0", &m_t_loc0);
  m_outputTree->Branch("t_loc1", &m_t_loc1);
  m_outputTree->Branch("t_phi", &m_t_phi);
  m_outputTree->Branch("t_theta", &m_t_theta);
  m_outputTree->Branch("t_qop", &m_t_qop);
  m_outputTree->Branch("t_time", &m_t_time);
  m_outputTree->Branch("t_charge", &m_t_charge);
  m_outputTree->Branch("t_p", &m_t_p);
  m_outputTree->Branch("t_pt", &m_t_pt);
  m_outputTree->Branch("t_eta", &m_t_eta);

  // The residuals
  m_outputTree->Branch("res_loc0", &m_res_loc0);
  m_outputTree->Branch("res_loc1", &m_res_loc1);
  m_outputTree->Branch("res_phi", &m_res_phi);
  m_outputTree->Branch("res_theta", &m_res_theta);
  m_outputTree->Branch("res_qop", &m_res_qop);
  m_outputTree->Branch("res_time", &m_res_time);

  // The pulls
  m_outputTree->Branch("pull_loc0", &m_pull_loc0);
  m_outputTree->Branch("pull_loc1", &m_pull_loc1);
  m_outputTree->Branch("pull_phi", &m_pull_phi);
  m_outputTree->Branch("pull_theta", &m_pull_theta);
  m_outputTree->Branch("pull_qop", &m_pull_qop);
  m_outputTree->Branch("pull_time", &m_pull_time);
}

RootTrackParameterWriter::~RootTrackParameterWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ProcessCode RootTrackParameterWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  ACTS_INFO("Wrote estimated parameters from seed to tree '"
            << m_cfg.treeName << "' in '" << m_cfg.filePath << "'");

  return ProcessCode::SUCCESS;
}

ProcessCode RootTrackParameterWriter::writeT(
    const AlgorithmContext& ctx, const TrackParametersContainer& trackParams) {
  // Read additional input collections
  const auto& protoTracks = m_inputProtoTracks(ctx);
  const auto& particles = m_inputParticles(ctx);
  const auto& simHits = m_inputSimHits(ctx);
  const auto& hitParticlesMap = m_inputMeasurementParticlesMap(ctx);
  const auto& hitSimHitsMap = m_inputMeasurementSimHitsMap(ctx);

  std::vector<ParticleHitCount> particleHitCounts;

  // Exclusive access to the tree while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  // Get the event number
  m_eventNr = ctx.eventNumber;

  ACTS_VERBOSE("Writing " << trackParams.size() << " track parameters");

  // Loop over the estimated track parameters
  for (std::size_t iparams = 0; iparams < trackParams.size(); ++iparams) {
    const auto& params = trackParams[iparams];
    // The reference surface of the parameters, i.e. also the reference surface
    // of the first space point
    const auto& surface = params.referenceSurface();

    m_volumeId = surface.geometryId().volume();
    m_layerId = surface.geometryId().layer();
    m_surfaceId = surface.geometryId().sensitive();

    m_loc0 = params.parameters()[Acts::eBoundLoc0];
    m_loc1 = params.parameters()[Acts::eBoundLoc1];
    m_phi = params.parameters()[Acts::eBoundPhi];
    m_theta = params.parameters()[Acts::eBoundTheta];
    m_qop = params.parameters()[Acts::eBoundQOverP];
    m_time = params.parameters()[Acts::eBoundTime];

    auto getError = [&params](std::size_t idx) -> float {
      if (!params.covariance().has_value()) {
        return NaNfloat;
      }
      const auto& cov = *params.covariance();
      if (cov(idx, idx) < 0) {
        return NaNfloat;
      }
      return std::sqrt(cov(idx, idx));
    };

    m_err_loc0 = getError(Acts::eBoundLoc0);
    m_err_loc1 = getError(Acts::eBoundLoc1);
    m_err_phi = getError(Acts::eBoundPhi);
    m_err_theta = getError(Acts::eBoundTheta);
    m_err_qop = getError(Acts::eBoundQOverP);
    m_err_time = getError(Acts::eBoundTime);

    m_charge = static_cast<int>(params.charge());
    m_p = params.absoluteMomentum();
    m_pt = params.transverseMomentum();
    m_eta = eta(params.momentum());

    // Get the proto track from which the track parameters are estimated
    const auto& ptrack = protoTracks[iparams];
    identifyContributingParticles(hitParticlesMap, ptrack, particleHitCounts);

    if (particleHitCounts.size() == 1) {
      m_t_matched = true;
      m_t_particleId = particleHitCounts.front().particleId.value();
      m_nMajorityHits = particleHitCounts.front().hitCount;

      // Get the index of the first space point
      const auto& hitIdx = ptrack.front();
      // Get the sim hits via the measurement to sim hits map
      auto indices = makeRange(hitSimHitsMap.equal_range(hitIdx));
      auto [truthLocal, truthPos4, truthUnitDir] =
          averageSimHits(ctx.geoContext, surface, simHits, indices, logger());

      // Get the truth track parameter at the first space point
      m_t_loc0 = truthLocal[Acts::ePos0];
      m_t_loc1 = truthLocal[Acts::ePos1];
      m_t_phi = phi(truthUnitDir);
      m_t_theta = theta(truthUnitDir);
      m_t_qop = NaNfloat;
      m_t_time = truthPos4[Acts::eTime];

      m_t_charge = 0;
      m_t_p = NaNfloat;
      m_t_pt = NaNfloat;
      m_t_eta = eta(truthUnitDir);

      // momentum averaging makes even less sense than averaging position and
      // direction. use the first momentum or set q/p to zero
      if (!indices.empty()) {
        // we assume that the indices are within valid ranges so we do not
        // need to check their validity again.
        const auto simHitIdx0 = indices.begin()->second;
        const auto& simHit0 = *simHits.nth(simHitIdx0);
        const auto p =
            simHit0.momentum4Before().template segment<3>(Acts::eMom0);
        const auto& particleId = simHit0.particleId();
        // The truth charge has to be retrieved from the sim particle
        if (auto ip = particles.find(particleId); ip != particles.end()) {
          const auto& particle = *ip;
          m_t_charge = static_cast<int>(particle.charge());
          m_t_qop = particle.hypothesis().qOverP(p.norm(), particle.charge());
          m_t_p = p.norm();
          m_t_pt = perp(p);
        } else {
          ACTS_WARNING("Truth particle with barcode " << particleId << "="
                                                      << particleId.value()
                                                      << " not found!");
        }
      }

      m_res_loc0 = m_loc0 - m_t_loc0;
      m_res_loc1 = m_loc1 - m_t_loc1;
      m_res_phi = Acts::detail::difference_periodic(
          m_phi, m_t_phi, static_cast<float>(2 * M_PI));
      m_res_theta = m_theta - m_t_theta;
      m_res_qop = m_qop - m_t_qop;
      m_res_time = m_time - m_t_time;

      auto getPull = [](float res, float err) -> float {
        if (std::isnan(err) || std::abs(err) < 1e-6) {
          return NaNfloat;
        }
        return res / err;
      };

      m_pull_loc0 = getPull(m_res_loc0, m_err_loc0);
      m_pull_loc1 = getPull(m_res_loc1, m_err_loc1);
      m_pull_phi = getPull(m_res_phi, m_err_phi);
      m_pull_theta = getPull(m_res_theta, m_err_theta);
      m_pull_qop = getPull(m_res_qop, m_err_qop);
      m_pull_time = getPull(m_res_time, m_err_time);
    } else {
      m_t_matched = false;
      m_t_particleId = 0;
      m_nMajorityHits = 0;

      m_t_loc0 = NaNfloat;
      m_t_loc1 = NaNfloat;
      m_t_phi = NaNfloat;
      m_t_theta = NaNfloat;
      m_t_qop = NaNfloat;
      m_t_time = NaNfloat;

      m_t_charge = 0;
      m_t_p = NaNfloat;
      m_t_pt = NaNfloat;
      m_t_eta = NaNfloat;
    }

    m_outputTree->Fill();
  }

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
