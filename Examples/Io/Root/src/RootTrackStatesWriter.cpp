// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootTrackStatesWriter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/MultiTrajectoryHelpers.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/TransformationHelpers.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/MultiIndex.hpp"
#include "Acts/Utilities/detail/periodic.hpp"
#include "ActsExamples/EventData/AverageSimHits.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Utilities/Range.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <cmath>
#include <cstddef>
#include <ios>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <utility>

#include <TFile.h>
#include <TTree.h>

namespace ActsExamples {

class IndexSourceLink;

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;
using Acts::VectorHelpers::theta;

RootTrackStatesWriter::RootTrackStatesWriter(
    const RootTrackStatesWriter::Config& config, Acts::Logging::Level level)
    : WriterT(config.inputTracks, "RootTrackStatesWriter", level),
      m_cfg(config) {
  // trajectories collection name is already checked by base ctor
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Missing particles input collection");
  }
  if (m_cfg.inputTrackParticleMatching.empty()) {
    throw std::invalid_argument("Missing input track particles matching");
  }
  if (m_cfg.inputSimHits.empty()) {
    throw std::invalid_argument("Missing simulated hits input collection");
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

  m_inputParticles.initialize(m_cfg.inputParticles);
  m_inputTrackParticleMatching.initialize(m_cfg.inputTrackParticleMatching);
  m_inputSimHits.initialize(m_cfg.inputSimHits);
  m_inputMeasurementSimHitsMap.initialize(m_cfg.inputMeasurementSimHitsMap);

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

  m_outputTree->Branch("volume_id", &m_volumeID);
  m_outputTree->Branch("layer_id", &m_layerID);
  m_outputTree->Branch("module_id", &m_moduleID);

  m_outputTree->Branch("stateType", &m_stateType);

  m_outputTree->Branch("chi2", &m_chi2);

  m_outputTree->Branch("pathLength", &m_pathLength);

  m_outputTree->Branch("t_x", &m_t_x);
  m_outputTree->Branch("t_y", &m_t_y);
  m_outputTree->Branch("t_z", &m_t_z);
  m_outputTree->Branch("t_r", &m_t_r);
  m_outputTree->Branch("t_dx", &m_t_dx);
  m_outputTree->Branch("t_dy", &m_t_dy);
  m_outputTree->Branch("t_dz", &m_t_dz);
  m_outputTree->Branch("t_eLOC0", &m_t_eLOC0);
  m_outputTree->Branch("t_eLOC1", &m_t_eLOC1);
  m_outputTree->Branch("t_ePHI", &m_t_ePHI);
  m_outputTree->Branch("t_eTHETA", &m_t_eTHETA);
  m_outputTree->Branch("t_eQOP", &m_t_eQOP);
  m_outputTree->Branch("t_eT", &m_t_eT);
  m_outputTree->Branch("particle_ids", &m_particleId);

  m_outputTree->Branch("dim_hit", &m_dim_hit);
  m_outputTree->Branch("l_x_hit", &m_lx_hit);
  m_outputTree->Branch("l_y_hit", &m_ly_hit);
  m_outputTree->Branch("g_x_hit", &m_x_hit);
  m_outputTree->Branch("g_y_hit", &m_y_hit);
  m_outputTree->Branch("g_z_hit", &m_z_hit);
  m_outputTree->Branch("res_x_hit", &m_res_x_hit);
  m_outputTree->Branch("res_y_hit", &m_res_y_hit);
  m_outputTree->Branch("err_x_hit", &m_err_x_hit);
  m_outputTree->Branch("err_y_hit", &m_err_y_hit);
  m_outputTree->Branch("pull_x_hit", &m_pull_x_hit);
  m_outputTree->Branch("pull_y_hit", &m_pull_y_hit);

  m_outputTree->Branch("nPredicted", &m_nParams[ePredicted]);
  m_outputTree->Branch("predicted", &m_hasParams[ePredicted]);
  m_outputTree->Branch("eLOC0_prt", &m_eLOC0[ePredicted]);
  m_outputTree->Branch("eLOC1_prt", &m_eLOC1[ePredicted]);
  m_outputTree->Branch("ePHI_prt", &m_ePHI[ePredicted]);
  m_outputTree->Branch("eTHETA_prt", &m_eTHETA[ePredicted]);
  m_outputTree->Branch("eQOP_prt", &m_eQOP[ePredicted]);
  m_outputTree->Branch("eT_prt", &m_eT[ePredicted]);
  m_outputTree->Branch("res_eLOC0_prt", &m_res_eLOC0[ePredicted]);
  m_outputTree->Branch("res_eLOC1_prt", &m_res_eLOC1[ePredicted]);
  m_outputTree->Branch("res_ePHI_prt", &m_res_ePHI[ePredicted]);
  m_outputTree->Branch("res_eTHETA_prt", &m_res_eTHETA[ePredicted]);
  m_outputTree->Branch("res_eQOP_prt", &m_res_eQOP[ePredicted]);
  m_outputTree->Branch("res_eT_prt", &m_res_eT[ePredicted]);
  m_outputTree->Branch("err_eLOC0_prt", &m_err_eLOC0[ePredicted]);
  m_outputTree->Branch("err_eLOC1_prt", &m_err_eLOC1[ePredicted]);
  m_outputTree->Branch("err_ePHI_prt", &m_err_ePHI[ePredicted]);
  m_outputTree->Branch("err_eTHETA_prt", &m_err_eTHETA[ePredicted]);
  m_outputTree->Branch("err_eQOP_prt", &m_err_eQOP[ePredicted]);
  m_outputTree->Branch("err_eT_prt", &m_err_eT[ePredicted]);
  m_outputTree->Branch("pull_eLOC0_prt", &m_pull_eLOC0[ePredicted]);
  m_outputTree->Branch("pull_eLOC1_prt", &m_pull_eLOC1[ePredicted]);
  m_outputTree->Branch("pull_ePHI_prt", &m_pull_ePHI[ePredicted]);
  m_outputTree->Branch("pull_eTHETA_prt", &m_pull_eTHETA[ePredicted]);
  m_outputTree->Branch("pull_eQOP_prt", &m_pull_eQOP[ePredicted]);
  m_outputTree->Branch("pull_eT_prt", &m_pull_eT[ePredicted]);
  m_outputTree->Branch("g_x_prt", &m_x[ePredicted]);
  m_outputTree->Branch("g_y_prt", &m_y[ePredicted]);
  m_outputTree->Branch("g_z_prt", &m_z[ePredicted]);
  m_outputTree->Branch("px_prt", &m_px[ePredicted]);
  m_outputTree->Branch("py_prt", &m_py[ePredicted]);
  m_outputTree->Branch("pz_prt", &m_pz[ePredicted]);
  m_outputTree->Branch("eta_prt", &m_eta[ePredicted]);
  m_outputTree->Branch("pT_prt", &m_pT[ePredicted]);

  m_outputTree->Branch("nFiltered", &m_nParams[eFiltered]);
  m_outputTree->Branch("filtered", &m_hasParams[eFiltered]);
  m_outputTree->Branch("eLOC0_flt", &m_eLOC0[eFiltered]);
  m_outputTree->Branch("eLOC1_flt", &m_eLOC1[eFiltered]);
  m_outputTree->Branch("ePHI_flt", &m_ePHI[eFiltered]);
  m_outputTree->Branch("eTHETA_flt", &m_eTHETA[eFiltered]);
  m_outputTree->Branch("eQOP_flt", &m_eQOP[eFiltered]);
  m_outputTree->Branch("eT_flt", &m_eT[eFiltered]);
  m_outputTree->Branch("res_eLOC0_flt", &m_res_eLOC0[eFiltered]);
  m_outputTree->Branch("res_eLOC1_flt", &m_res_eLOC1[eFiltered]);
  m_outputTree->Branch("res_ePHI_flt", &m_res_ePHI[eFiltered]);
  m_outputTree->Branch("res_eTHETA_flt", &m_res_eTHETA[eFiltered]);
  m_outputTree->Branch("res_eQOP_flt", &m_res_eQOP[eFiltered]);
  m_outputTree->Branch("res_eT_flt", &m_res_eT[eFiltered]);
  m_outputTree->Branch("err_eLOC0_flt", &m_err_eLOC0[eFiltered]);
  m_outputTree->Branch("err_eLOC1_flt", &m_err_eLOC1[eFiltered]);
  m_outputTree->Branch("err_ePHI_flt", &m_err_ePHI[eFiltered]);
  m_outputTree->Branch("err_eTHETA_flt", &m_err_eTHETA[eFiltered]);
  m_outputTree->Branch("err_eQOP_flt", &m_err_eQOP[eFiltered]);
  m_outputTree->Branch("err_eT_flt", &m_err_eT[eFiltered]);
  m_outputTree->Branch("pull_eLOC0_flt", &m_pull_eLOC0[eFiltered]);
  m_outputTree->Branch("pull_eLOC1_flt", &m_pull_eLOC1[eFiltered]);
  m_outputTree->Branch("pull_ePHI_flt", &m_pull_ePHI[eFiltered]);
  m_outputTree->Branch("pull_eTHETA_flt", &m_pull_eTHETA[eFiltered]);
  m_outputTree->Branch("pull_eQOP_flt", &m_pull_eQOP[eFiltered]);
  m_outputTree->Branch("pull_eT_flt", &m_pull_eT[eFiltered]);
  m_outputTree->Branch("g_x_flt", &m_x[eFiltered]);
  m_outputTree->Branch("g_y_flt", &m_y[eFiltered]);
  m_outputTree->Branch("g_z_flt", &m_z[eFiltered]);
  m_outputTree->Branch("px_flt", &m_px[eFiltered]);
  m_outputTree->Branch("py_flt", &m_py[eFiltered]);
  m_outputTree->Branch("pz_flt", &m_pz[eFiltered]);
  m_outputTree->Branch("eta_flt", &m_eta[eFiltered]);
  m_outputTree->Branch("pT_flt", &m_pT[eFiltered]);

  m_outputTree->Branch("nSmoothed", &m_nParams[eSmoothed]);
  m_outputTree->Branch("smoothed", &m_hasParams[eSmoothed]);
  m_outputTree->Branch("eLOC0_smt", &m_eLOC0[eSmoothed]);
  m_outputTree->Branch("eLOC1_smt", &m_eLOC1[eSmoothed]);
  m_outputTree->Branch("ePHI_smt", &m_ePHI[eSmoothed]);
  m_outputTree->Branch("eTHETA_smt", &m_eTHETA[eSmoothed]);
  m_outputTree->Branch("eQOP_smt", &m_eQOP[eSmoothed]);
  m_outputTree->Branch("eT_smt", &m_eT[eSmoothed]);
  m_outputTree->Branch("res_eLOC0_smt", &m_res_eLOC0[eSmoothed]);
  m_outputTree->Branch("res_eLOC1_smt", &m_res_eLOC1[eSmoothed]);
  m_outputTree->Branch("res_ePHI_smt", &m_res_ePHI[eSmoothed]);
  m_outputTree->Branch("res_eTHETA_smt", &m_res_eTHETA[eSmoothed]);
  m_outputTree->Branch("res_eQOP_smt", &m_res_eQOP[eSmoothed]);
  m_outputTree->Branch("res_eT_smt", &m_res_eT[eSmoothed]);
  m_outputTree->Branch("err_eLOC0_smt", &m_err_eLOC0[eSmoothed]);
  m_outputTree->Branch("err_eLOC1_smt", &m_err_eLOC1[eSmoothed]);
  m_outputTree->Branch("err_ePHI_smt", &m_err_ePHI[eSmoothed]);
  m_outputTree->Branch("err_eTHETA_smt", &m_err_eTHETA[eSmoothed]);
  m_outputTree->Branch("err_eQOP_smt", &m_err_eQOP[eSmoothed]);
  m_outputTree->Branch("err_eT_smt", &m_err_eT[eSmoothed]);
  m_outputTree->Branch("pull_eLOC0_smt", &m_pull_eLOC0[eSmoothed]);
  m_outputTree->Branch("pull_eLOC1_smt", &m_pull_eLOC1[eSmoothed]);
  m_outputTree->Branch("pull_ePHI_smt", &m_pull_ePHI[eSmoothed]);
  m_outputTree->Branch("pull_eTHETA_smt", &m_pull_eTHETA[eSmoothed]);
  m_outputTree->Branch("pull_eQOP_smt", &m_pull_eQOP[eSmoothed]);
  m_outputTree->Branch("pull_eT_smt", &m_pull_eT[eSmoothed]);
  m_outputTree->Branch("g_x_smt", &m_x[eSmoothed]);
  m_outputTree->Branch("g_y_smt", &m_y[eSmoothed]);
  m_outputTree->Branch("g_z_smt", &m_z[eSmoothed]);
  m_outputTree->Branch("px_smt", &m_px[eSmoothed]);
  m_outputTree->Branch("py_smt", &m_py[eSmoothed]);
  m_outputTree->Branch("pz_smt", &m_pz[eSmoothed]);
  m_outputTree->Branch("eta_smt", &m_eta[eSmoothed]);
  m_outputTree->Branch("pT_smt", &m_pT[eSmoothed]);

  m_outputTree->Branch("nUnbiased", &m_nParams[eUnbiased]);
  m_outputTree->Branch("unbiased", &m_hasParams[eUnbiased]);
  m_outputTree->Branch("eLOC0_ubs", &m_eLOC0[eUnbiased]);
  m_outputTree->Branch("eLOC1_ubs", &m_eLOC1[eUnbiased]);
  m_outputTree->Branch("ePHI_ubs", &m_ePHI[eUnbiased]);
  m_outputTree->Branch("eTHETA_ubs", &m_eTHETA[eUnbiased]);
  m_outputTree->Branch("eQOP_ubs", &m_eQOP[eUnbiased]);
  m_outputTree->Branch("eT_ubs", &m_eT[eUnbiased]);
  m_outputTree->Branch("res_eLOC0_ubs", &m_res_eLOC0[eUnbiased]);
  m_outputTree->Branch("res_eLOC1_ubs", &m_res_eLOC1[eUnbiased]);
  m_outputTree->Branch("res_ePHI_ubs", &m_res_ePHI[eUnbiased]);
  m_outputTree->Branch("res_eTHETA_ubs", &m_res_eTHETA[eUnbiased]);
  m_outputTree->Branch("res_eQOP_ubs", &m_res_eQOP[eUnbiased]);
  m_outputTree->Branch("res_eT_ubs", &m_res_eT[eUnbiased]);
  m_outputTree->Branch("err_eLOC0_ubs", &m_err_eLOC0[eUnbiased]);
  m_outputTree->Branch("err_eLOC1_ubs", &m_err_eLOC1[eUnbiased]);
  m_outputTree->Branch("err_ePHI_ubs", &m_err_ePHI[eUnbiased]);
  m_outputTree->Branch("err_eTHETA_ubs", &m_err_eTHETA[eUnbiased]);
  m_outputTree->Branch("err_eQOP_ubs", &m_err_eQOP[eUnbiased]);
  m_outputTree->Branch("err_eT_ubs", &m_err_eT[eUnbiased]);
  m_outputTree->Branch("pull_eLOC0_ubs", &m_pull_eLOC0[eUnbiased]);
  m_outputTree->Branch("pull_eLOC1_ubs", &m_pull_eLOC1[eUnbiased]);
  m_outputTree->Branch("pull_ePHI_ubs", &m_pull_ePHI[eUnbiased]);
  m_outputTree->Branch("pull_eTHETA_ubs", &m_pull_eTHETA[eUnbiased]);
  m_outputTree->Branch("pull_eQOP_ubs", &m_pull_eQOP[eUnbiased]);
  m_outputTree->Branch("pull_eT_ubs", &m_pull_eT[eUnbiased]);
  m_outputTree->Branch("g_x_ubs", &m_x[eUnbiased]);
  m_outputTree->Branch("g_y_ubs", &m_y[eUnbiased]);
  m_outputTree->Branch("g_z_ubs", &m_z[eUnbiased]);
  m_outputTree->Branch("px_ubs", &m_px[eUnbiased]);
  m_outputTree->Branch("py_ubs", &m_py[eUnbiased]);
  m_outputTree->Branch("pz_ubs", &m_pz[eUnbiased]);
  m_outputTree->Branch("eta_ubs", &m_eta[eUnbiased]);
  m_outputTree->Branch("pT_ubs", &m_pT[eUnbiased]);
}

RootTrackStatesWriter::~RootTrackStatesWriter() {
  m_outputFile->Close();
}

ProcessCode RootTrackStatesWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  ACTS_INFO("Wrote states of trajectories to tree '"
            << m_cfg.treeName << "' in '" << m_cfg.treeName << "'");

  return ProcessCode::SUCCESS;
}

RootTrackStatesWriter::StateType RootTrackStatesWriter::getStateType(
    ConstTrackStateProxy state) {
  if (state.typeFlags().test(Acts::OutlierFlag)) {
    return StateType::eOutlier;
  }
  if (state.typeFlags().test(Acts::MeasurementFlag)) {
    return StateType::eMeasurement;
  }
  if (state.typeFlags().test(Acts::HoleFlag)) {
    return StateType::eHole;
  }
  if (state.typeFlags().test(Acts::MaterialFlag)) {
    return StateType::eMaterial;
  }
  return StateType::eUnknown;
}

ProcessCode RootTrackStatesWriter::writeT(const AlgorithmContext& ctx,
                                          const ConstTrackContainer& tracks) {
  float nan = std::numeric_limits<float>::quiet_NaN();

  auto& gctx = ctx.geoContext;
  // Read additional input collections
  const auto& particles = m_inputParticles(ctx);
  const auto& trackParticleMatching = m_inputTrackParticleMatching(ctx);
  const auto& simHits = m_inputSimHits(ctx);
  const auto& hitSimHitsMap = m_inputMeasurementSimHitsMap(ctx);

  // Exclusive access to the tree while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  // Get the event number
  m_eventNr = ctx.eventNumber;

  for (const auto& track : tracks) {
    m_trackNr = track.index();

    // Collect the track summary info
    m_nMeasurements = track.nMeasurements();
    m_nStates = track.nTrackStates();

    // Get the majority truth particle to this track
    int truthQ = 1.;
    auto match = trackParticleMatching.find(track.index());
    if (match != trackParticleMatching.end() &&
        match->second.particle.has_value()) {
      // Get the barcode of the majority truth particle
      auto barcode = match->second.particle.value();
      // Find the truth particle via the barcode
      auto ip = particles.find(barcode);
      if (ip != particles.end()) {
        const auto& particle = *ip;
        ACTS_VERBOSE("Find the truth particle with barcode "
                     << barcode << "=" << barcode.value());
        // Get the truth particle charge
        truthQ = static_cast<int>(particle.charge());
      } else {
        ACTS_DEBUG("Truth particle with barcode "
                   << barcode << "=" << barcode.value() << " not found!");
      }
    }

    // Get the trackStates on the trajectory
    m_nParams = {0, 0, 0, 0};

    // particle barcodes for a given track state (size depends on a type of
    // digitization, for smeared digitization is not more than 1)
    std::vector<std::uint64_t> particleIds;

    for (const auto& state : track.trackStatesReversed()) {
      const auto& surface = state.referenceSurface();

      // get the geometry ID
      auto geoID = surface.geometryId();
      m_volumeID.push_back(geoID.volume());
      m_layerID.push_back(geoID.layer());
      m_moduleID.push_back(geoID.sensitive());

      m_stateType.push_back(Acts::toUnderlying(getStateType(state)));

      // get the path length
      m_pathLength.push_back(state.pathLength());

      // fill the chi2
      m_chi2.push_back(state.chi2());

      // the truth track parameter at this track state
      Acts::BoundVector truthParams;

      particleIds.clear();

      if (!state.hasUncalibratedSourceLink()) {
        m_t_x.push_back(nan);
        m_t_y.push_back(nan);
        m_t_z.push_back(nan);
        m_t_r.push_back(nan);
        m_t_dx.push_back(nan);
        m_t_dy.push_back(nan);
        m_t_dz.push_back(nan);
        m_t_eLOC0.push_back(nan);
        m_t_eLOC1.push_back(nan);
        m_t_ePHI.push_back(nan);
        m_t_eTHETA.push_back(nan);
        m_t_eQOP.push_back(nan);
        m_t_eT.push_back(nan);

        m_lx_hit.push_back(nan);
        m_ly_hit.push_back(nan);
        m_x_hit.push_back(nan);
        m_y_hit.push_back(nan);
        m_z_hit.push_back(nan);
      } else {
        // get the truth hits corresponding to this trackState
        // Use average truth in the case of multiple contributing sim hits
        auto sl =
            state.getUncalibratedSourceLink().template get<IndexSourceLink>();
        const auto hitIdx = sl.index();
        auto indices = makeRange(hitSimHitsMap.equal_range(hitIdx));
        auto [truthLocal, truthPos4, truthUnitDir] =
            averageSimHits(ctx.geoContext, surface, simHits, indices, logger());
        // momentum averaging makes even less sense than averaging position and
        // direction. use the first momentum or set q/p to zero
        if (!indices.empty()) {
          // we assume that the indices are within valid ranges so we do not
          // need to check their validity again.
          const auto simHitIdx0 = indices.begin()->second;
          const auto& simHit0 = *simHits.nth(simHitIdx0);
          const auto p =
              simHit0.momentum4Before().template segment<3>(Acts::eMom0).norm();
          truthParams[Acts::eBoundQOverP] = truthQ / p;

          // extract particle ids contributed to this track state
          for (auto const& [key, simHitIdx] : indices) {
            const auto& simHit = *simHits.nth(simHitIdx);
            particleIds.push_back(simHit.particleId().value());
          }
        }

        // fill the truth hit info
        m_t_x.push_back(static_cast<float>(truthPos4[Acts::ePos0]));
        m_t_y.push_back(static_cast<float>(truthPos4[Acts::ePos1]));
        m_t_z.push_back(static_cast<float>(truthPos4[Acts::ePos2]));
        m_t_r.push_back(static_cast<float>(
            perp(truthPos4.template segment<3>(Acts::ePos0))));
        m_t_dx.push_back(static_cast<float>(truthUnitDir[Acts::eMom0]));
        m_t_dy.push_back(static_cast<float>(truthUnitDir[Acts::eMom1]));
        m_t_dz.push_back(static_cast<float>(truthUnitDir[Acts::eMom2]));

        // get the truth track parameter at this track State
        truthParams[Acts::eBoundLoc0] = truthLocal[Acts::ePos0];
        truthParams[Acts::eBoundLoc1] = truthLocal[Acts::ePos1];
        truthParams[Acts::eBoundPhi] = phi(truthUnitDir);
        truthParams[Acts::eBoundTheta] = theta(truthUnitDir);
        truthParams[Acts::eBoundTime] = truthPos4[Acts::eTime];

        // fill the truth track parameter at this track State
        m_t_eLOC0.push_back(static_cast<float>(truthParams[Acts::eBoundLoc0]));
        m_t_eLOC1.push_back(static_cast<float>(truthParams[Acts::eBoundLoc1]));
        m_t_ePHI.push_back(static_cast<float>(truthParams[Acts::eBoundPhi]));
        m_t_eTHETA.push_back(
            static_cast<float>(truthParams[Acts::eBoundTheta]));
        m_t_eQOP.push_back(static_cast<float>(truthParams[Acts::eBoundQOverP]));
        m_t_eT.push_back(static_cast<float>(truthParams[Acts::eBoundTime]));

        // expand the local measurements into the full bound space
        Acts::BoundVector meas = state.effectiveProjector().transpose() *
                                 state.effectiveCalibrated();
        // extract local and global position
        Acts::Vector2 local(meas[Acts::eBoundLoc0], meas[Acts::eBoundLoc1]);
        Acts::Vector3 global =
            surface.localToGlobal(ctx.geoContext, local, truthUnitDir);

        // fill the measurement info
        m_lx_hit.push_back(static_cast<float>(local[Acts::ePos0]));
        m_ly_hit.push_back(static_cast<float>(local[Acts::ePos1]));
        m_x_hit.push_back(static_cast<float>(global[Acts::ePos0]));
        m_y_hit.push_back(static_cast<float>(global[Acts::ePos1]));
        m_z_hit.push_back(static_cast<float>(global[Acts::ePos2]));
      }

      // lambda to get the fitted track parameters
      auto getTrackParams = [&](unsigned int ipar)
          -> std::optional<std::pair<Acts::BoundVector, Acts::BoundMatrix>> {
        if (ipar == ePredicted && state.hasPredicted()) {
          return std::make_pair(state.predicted(), state.predictedCovariance());
        }
        if (ipar == eFiltered && state.hasFiltered()) {
          return std::make_pair(state.filtered(), state.filteredCovariance());
        }
        if (ipar == eSmoothed && state.hasSmoothed()) {
          return std::make_pair(state.smoothed(), state.smoothedCovariance());
        }
        if (ipar == eUnbiased && state.hasSmoothed() && state.hasProjector() &&
            state.hasCalibrated()) {
          // calculate the unbiased track parameters (i.e. fitted track
          // parameters with this measurement removed) using Eq.(12a)-Eq.(12c)
          // of NIMA 262, 444 (1987)
          auto m = state.effectiveCalibrated();
          auto H = state.effectiveProjector();
          auto V = state.effectiveCalibratedCovariance();
          auto K =
              (state.smoothedCovariance() * H.transpose() *
               (H * state.smoothedCovariance() * H.transpose() - V).inverse())
                  .eval();
          auto unbiasedParamsVec =
              state.smoothed() + K * (m - H * state.smoothed());
          auto unbiasedParamsCov =
              state.smoothedCovariance() - K * H * state.smoothedCovariance();
          return std::make_pair(unbiasedParamsVec, unbiasedParamsCov);
        }
        return std::nullopt;
      };

      // fill the fitted track parameters
      for (unsigned int ipar = 0; ipar < eSize; ++ipar) {
        // get the fitted track parameters
        auto trackParamsOpt = getTrackParams(ipar);
        // fill the track parameters status
        m_hasParams[ipar].push_back(trackParamsOpt.has_value());

        if (!trackParamsOpt.has_value()) {
          if (ipar == ePredicted) {
            // push default values if no track parameters
            m_res_x_hit.push_back(nan);
            m_res_y_hit.push_back(nan);
            m_err_x_hit.push_back(nan);
            m_err_y_hit.push_back(nan);
            m_pull_x_hit.push_back(nan);
            m_pull_y_hit.push_back(nan);
            m_dim_hit.push_back(0);
          }

          // push default values if no track parameters
          m_eLOC0[ipar].push_back(nan);
          m_eLOC1[ipar].push_back(nan);
          m_ePHI[ipar].push_back(nan);
          m_eTHETA[ipar].push_back(nan);
          m_eQOP[ipar].push_back(nan);
          m_eT[ipar].push_back(nan);
          m_res_eLOC0[ipar].push_back(nan);
          m_res_eLOC1[ipar].push_back(nan);
          m_res_ePHI[ipar].push_back(nan);
          m_res_eTHETA[ipar].push_back(nan);
          m_res_eQOP[ipar].push_back(nan);
          m_res_eT[ipar].push_back(nan);
          m_err_eLOC0[ipar].push_back(nan);
          m_err_eLOC1[ipar].push_back(nan);
          m_err_ePHI[ipar].push_back(nan);
          m_err_eTHETA[ipar].push_back(nan);
          m_err_eQOP[ipar].push_back(nan);
          m_err_eT[ipar].push_back(nan);
          m_pull_eLOC0[ipar].push_back(nan);
          m_pull_eLOC1[ipar].push_back(nan);
          m_pull_ePHI[ipar].push_back(nan);
          m_pull_eTHETA[ipar].push_back(nan);
          m_pull_eQOP[ipar].push_back(nan);
          m_pull_eT[ipar].push_back(nan);
          m_x[ipar].push_back(nan);
          m_y[ipar].push_back(nan);
          m_z[ipar].push_back(nan);
          m_px[ipar].push_back(nan);
          m_py[ipar].push_back(nan);
          m_pz[ipar].push_back(nan);
          m_pT[ipar].push_back(nan);
          m_eta[ipar].push_back(nan);

          continue;
        }

        ++m_nParams[ipar];
        const auto& [parameters, covariance] = *trackParamsOpt;

        // track parameters
        m_eLOC0[ipar].push_back(
            static_cast<float>(parameters[Acts::eBoundLoc0]));
        m_eLOC1[ipar].push_back(
            static_cast<float>(parameters[Acts::eBoundLoc1]));
        m_ePHI[ipar].push_back(static_cast<float>(parameters[Acts::eBoundPhi]));
        m_eTHETA[ipar].push_back(
            static_cast<float>(parameters[Acts::eBoundTheta]));
        m_eQOP[ipar].push_back(
            static_cast<float>(parameters[Acts::eBoundQOverP]));
        m_eT[ipar].push_back(static_cast<float>(parameters[Acts::eBoundTime]));

        // track parameters error
        Acts::BoundVector errors;
        for (Eigen::Index i = 0; i < parameters.size(); ++i) {
          double variance = covariance(i, i);
          errors[i] = variance >= 0 ? std::sqrt(variance) : nan;
        }
        m_err_eLOC0[ipar].push_back(
            static_cast<float>(errors[Acts::eBoundLoc0]));
        m_err_eLOC1[ipar].push_back(
            static_cast<float>(errors[Acts::eBoundLoc1]));
        m_err_ePHI[ipar].push_back(static_cast<float>(errors[Acts::eBoundPhi]));
        m_err_eTHETA[ipar].push_back(
            static_cast<float>(errors[Acts::eBoundTheta]));
        m_err_eQOP[ipar].push_back(
            static_cast<float>(errors[Acts::eBoundQOverP]));
        m_err_eT[ipar].push_back(static_cast<float>(errors[Acts::eBoundTime]));

        // further track parameter info
        Acts::FreeVector freeParams =
            Acts::transformBoundToFreeParameters(surface, gctx, parameters);
        m_x[ipar].push_back(freeParams[Acts::eFreePos0]);
        m_y[ipar].push_back(freeParams[Acts::eFreePos1]);
        m_z[ipar].push_back(freeParams[Acts::eFreePos2]);
        // single charge assumption
        auto p = std::abs(1 / freeParams[Acts::eFreeQOverP]);
        m_px[ipar].push_back(p * freeParams[Acts::eFreeDir0]);
        m_py[ipar].push_back(p * freeParams[Acts::eFreeDir1]);
        m_pz[ipar].push_back(p * freeParams[Acts::eFreeDir2]);
        m_pT[ipar].push_back(p * std::hypot(freeParams[Acts::eFreeDir0],
                                            freeParams[Acts::eFreeDir1]));
        m_eta[ipar].push_back(
            Acts::VectorHelpers::eta(freeParams.segment<3>(Acts::eFreeDir0)));

        if (!state.hasUncalibratedSourceLink()) {
          continue;
        }

        // track parameters residual
        Acts::BoundVector residuals;
        residuals = parameters - truthParams;
        residuals[Acts::eBoundPhi] = Acts::detail::difference_periodic(
            parameters[Acts::eBoundPhi], truthParams[Acts::eBoundPhi],
            2 * M_PI);
        m_res_eLOC0[ipar].push_back(
            static_cast<float>(residuals[Acts::eBoundLoc0]));
        m_res_eLOC1[ipar].push_back(
            static_cast<float>(residuals[Acts::eBoundLoc1]));
        m_res_ePHI[ipar].push_back(
            static_cast<float>(residuals[Acts::eBoundPhi]));
        m_res_eTHETA[ipar].push_back(
            static_cast<float>(residuals[Acts::eBoundTheta]));
        m_res_eQOP[ipar].push_back(
            static_cast<float>(residuals[Acts::eBoundQOverP]));
        m_res_eT[ipar].push_back(
            static_cast<float>(residuals[Acts::eBoundTime]));

        // track parameters pull
        Acts::BoundVector pulls;
        for (Eigen::Index i = 0; i < parameters.size(); ++i) {
          pulls[i] = (!std::isnan(errors[i]) && errors[i] > 0)
                         ? residuals[i] / errors[i]
                         : nan;
        }
        m_pull_eLOC0[ipar].push_back(
            static_cast<float>(pulls[Acts::eBoundLoc0]));
        m_pull_eLOC1[ipar].push_back(
            static_cast<float>(pulls[Acts::eBoundLoc1]));
        m_pull_ePHI[ipar].push_back(static_cast<float>(pulls[Acts::eBoundPhi]));
        m_pull_eTHETA[ipar].push_back(
            static_cast<float>(pulls[Acts::eBoundTheta]));
        m_pull_eQOP[ipar].push_back(
            static_cast<float>(pulls[Acts::eBoundQOverP]));
        m_pull_eT[ipar].push_back(static_cast<float>(pulls[Acts::eBoundTime]));

        if (ipar == ePredicted) {
          // local hit residual info
          auto H = state.effectiveProjector();
          auto V = state.effectiveCalibratedCovariance();
          auto resCov = V + H * covariance * H.transpose();
          Acts::ActsDynamicVector res =
              state.effectiveCalibrated() - H * parameters;

          double resX = res[Acts::eBoundLoc0];
          double errX = V(Acts::eBoundLoc0, Acts::eBoundLoc0) >= 0
                            ? std::sqrt(V(Acts::eBoundLoc0, Acts::eBoundLoc0))
                            : nan;
          double pullX =
              resCov(Acts::eBoundLoc0, Acts::eBoundLoc0) > 0
                  ? resX / std::sqrt(resCov(Acts::eBoundLoc0, Acts::eBoundLoc0))
                  : nan;

          m_res_x_hit.push_back(static_cast<float>(resX));
          m_err_x_hit.push_back(static_cast<float>(errX));
          m_pull_x_hit.push_back(static_cast<float>(pullX));

          if (state.calibratedSize() >= 2) {
            double resY = res[Acts::eBoundLoc1];
            double errY = V(Acts::eBoundLoc1, Acts::eBoundLoc1) >= 0
                              ? std::sqrt(V(Acts::eBoundLoc1, Acts::eBoundLoc1))
                              : nan;
            double pullY = resCov(Acts::eBoundLoc1, Acts::eBoundLoc1) > 0
                               ? resY / std::sqrt(resCov(Acts::eBoundLoc1,
                                                         Acts::eBoundLoc1))
                               : nan;

            m_res_y_hit.push_back(static_cast<float>(resY));
            m_err_y_hit.push_back(static_cast<float>(errY));
            m_pull_y_hit.push_back(static_cast<float>(pullY));
          } else {
            m_res_y_hit.push_back(nan);
            m_err_y_hit.push_back(nan);
            m_pull_y_hit.push_back(nan);
          }

          m_dim_hit.push_back(state.calibratedSize());
        }
      }
      m_particleId.push_back(std::move(particleIds));
    }

    // fill the variables for one track to tree
    m_outputTree->Fill();

    // now reset
    m_volumeID.clear();
    m_layerID.clear();
    m_moduleID.clear();

    m_stateType.clear();

    m_chi2.clear();

    m_pathLength.clear();

    m_t_x.clear();
    m_t_y.clear();
    m_t_z.clear();
    m_t_r.clear();
    m_t_dx.clear();
    m_t_dy.clear();
    m_t_dz.clear();
    m_t_eLOC0.clear();
    m_t_eLOC1.clear();
    m_t_ePHI.clear();
    m_t_eTHETA.clear();
    m_t_eQOP.clear();
    m_t_eT.clear();

    m_particleId.clear();

    m_dim_hit.clear();
    m_lx_hit.clear();
    m_ly_hit.clear();
    m_x_hit.clear();
    m_y_hit.clear();
    m_z_hit.clear();
    m_res_x_hit.clear();
    m_res_y_hit.clear();
    m_err_x_hit.clear();
    m_err_y_hit.clear();
    m_pull_x_hit.clear();
    m_pull_y_hit.clear();

    for (unsigned int ipar = 0; ipar < eSize; ++ipar) {
      m_hasParams[ipar].clear();
      m_eLOC0[ipar].clear();
      m_eLOC1[ipar].clear();
      m_ePHI[ipar].clear();
      m_eTHETA[ipar].clear();
      m_eQOP[ipar].clear();
      m_eT[ipar].clear();
      m_res_eLOC0[ipar].clear();
      m_res_eLOC1[ipar].clear();
      m_res_ePHI[ipar].clear();
      m_res_eTHETA[ipar].clear();
      m_res_eQOP[ipar].clear();
      m_res_eT[ipar].clear();
      m_err_eLOC0[ipar].clear();
      m_err_eLOC1[ipar].clear();
      m_err_ePHI[ipar].clear();
      m_err_eTHETA[ipar].clear();
      m_err_eQOP[ipar].clear();
      m_err_eT[ipar].clear();
      m_pull_eLOC0[ipar].clear();
      m_pull_eLOC1[ipar].clear();
      m_pull_ePHI[ipar].clear();
      m_pull_eTHETA[ipar].clear();
      m_pull_eQOP[ipar].clear();
      m_pull_eT[ipar].clear();
      m_x[ipar].clear();
      m_y[ipar].clear();
      m_z[ipar].clear();
      m_px[ipar].clear();
      m_py[ipar].clear();
      m_pz[ipar].clear();
      m_eta[ipar].clear();
      m_pT[ipar].clear();
    }

    m_chi2.clear();
  }

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
