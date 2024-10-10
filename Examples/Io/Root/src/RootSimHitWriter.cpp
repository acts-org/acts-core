// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootSimHitWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Hit.hpp"

#include <ios>
#include <ostream>
#include <stdexcept>

#include <TFile.h>
#include <TTree.h>

ActsExamples::RootSimHitWriter::RootSimHitWriter(
    const ActsExamples::RootSimHitWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputSimHits, "RootSimHitWriter", level), m_cfg(config) {
  // inputParticles is already checked by base constructor
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing file path");
  }
  if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }

  // open root file and create the tree
  m_outputFile = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
  if (m_outputFile == nullptr) {
    throw std::ios_base::failure("Could not open '" + m_cfg.filePath + "'");
  }
  m_outputFile->cd();
  m_outputTree = new TTree(m_cfg.treeName.c_str(), m_cfg.treeName.c_str());
  if (m_outputTree == nullptr) {
    throw std::bad_alloc();
  }

  // setup the branches
  m_outputTree->Branch("event_id", &m_eventId);
  m_outputTree->Branch("geometry_id", &m_geometryId, "geometry_id/l");
  m_outputTree->Branch("particle_id", &m_particleId, "particle_id/l");
  m_outputTree->Branch("tx", &m_tx);
  m_outputTree->Branch("ty", &m_ty);
  m_outputTree->Branch("tz", &m_tz);
  m_outputTree->Branch("tt", &m_tt);
  m_outputTree->Branch("tpx", &m_tpx);
  m_outputTree->Branch("tpy", &m_tpy);
  m_outputTree->Branch("tpz", &m_tpz);
  m_outputTree->Branch("te", &m_te);
  m_outputTree->Branch("deltapx", &m_deltapx);
  m_outputTree->Branch("deltapy", &m_deltapy);
  m_outputTree->Branch("deltapz", &m_deltapz);
  m_outputTree->Branch("deltae", &m_deltae);
  m_outputTree->Branch("index", &m_index);
  m_outputTree->Branch("volume_id", &m_volumeId);
  m_outputTree->Branch("boundary_id", &m_boundaryId);
  m_outputTree->Branch("layer_id", &m_layerId);
  m_outputTree->Branch("approach_id", &m_approachId);
  m_outputTree->Branch("sensitive_id", &m_sensitiveId);
}

ActsExamples::RootSimHitWriter::~RootSimHitWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ActsExamples::ProcessCode ActsExamples::RootSimHitWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  ACTS_VERBOSE("Wrote hits to tree '" << m_cfg.treeName << "' in '"
                                      << m_cfg.filePath << "'");

  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::RootSimHitWriter::writeT(
    const AlgorithmContext& ctx, const ActsExamples::SimHitContainer& hits) {
  // ensure exclusive access to tree/file while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  // Get the event number
  m_eventId = ctx.eventNumber;
  for (const auto& hit : hits) {
    m_particleId = hit.particleId().value();
    m_geometryId = hit.geometryId().value();
    // write hit position
    m_tx = hit.fourPosition().x() / Acts::UnitConstants::mm;
    m_ty = hit.fourPosition().y() / Acts::UnitConstants::mm;
    m_tz = hit.fourPosition().z() / Acts::UnitConstants::mm;
    m_tt = hit.fourPosition().w() / Acts::UnitConstants::mm;
    // write four-momentum before interaction
    m_tpx = hit.momentum4Before().x() / Acts::UnitConstants::GeV;
    m_tpy = hit.momentum4Before().y() / Acts::UnitConstants::GeV;
    m_tpz = hit.momentum4Before().z() / Acts::UnitConstants::GeV;
    m_te = hit.momentum4Before().w() / Acts::UnitConstants::GeV;
    // write four-momentum change due to interaction
    const auto delta4 = hit.momentum4After() - hit.momentum4Before();
    m_deltapx = delta4.x() / Acts::UnitConstants::GeV;
    m_deltapy = delta4.y() / Acts::UnitConstants::GeV;
    m_deltapz = delta4.z() / Acts::UnitConstants::GeV;
    m_deltae = delta4.w() / Acts::UnitConstants::GeV;
    // write hit index along trajectory
    m_index = hit.index();
    // decoded geometry for simplicity
    m_volumeId = hit.geometryId().volume();
    m_boundaryId = hit.geometryId().boundary();
    m_layerId = hit.geometryId().layer();
    m_approachId = hit.geometryId().approach();
    m_sensitiveId = hit.geometryId().sensitive();
    // Fill the tree
    m_outputTree->Fill();
  }
  return ActsExamples::ProcessCode::SUCCESS;
}
