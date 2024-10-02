// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootVertexWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <algorithm>
#include <cstdint>
#include <ios>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <unordered_map>

#include <TFile.h>
#include <TTree.h>

namespace ActsExamples {

RootVertexWriter::RootVertexWriter(const RootVertexWriter::Config& cfg,
                                   Acts::Logging::Level lvl)
    : WriterT(cfg.inputVertices, "RootVertexWriter", lvl), m_cfg(cfg) {
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
  m_outputTree->Branch("vertex_id", &m_vertexId);
  m_outputTree->Branch("process", &m_process);
  m_outputTree->Branch("vx", &m_vx);
  m_outputTree->Branch("vy", &m_vy);
  m_outputTree->Branch("vz", &m_vz);
  m_outputTree->Branch("vt", &m_vt);
  m_outputTree->Branch("outgoing_particles", &m_outgoingParticles);
  m_outputTree->Branch("vertex_primary", &m_vertexPrimary);
  m_outputTree->Branch("vertex_secondary", &m_vertexSecondary);
  m_outputTree->Branch("generation", &m_generation);
}

RootVertexWriter::~RootVertexWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ProcessCode RootVertexWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  ACTS_INFO("Wrote vertices to tree '" << m_cfg.treeName << "' in '"
                                       << m_cfg.filePath << "'");

  return ProcessCode::SUCCESS;
}

ProcessCode RootVertexWriter::writeT(const AlgorithmContext& ctx,
                                     const SimVertexContainer& vertices) {
  // ensure exclusive access to tree/file while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  m_eventId = ctx.eventNumber;
  for (const auto& vertex : vertices) {
    m_vertexId.push_back(vertex.vertexId().value());
    m_process.push_back(static_cast<std::uint32_t>(vertex.process));
    // position
    m_vx.push_back(Acts::clampValue<float>(vertex.position4.x() /
                                           Acts::UnitConstants::mm));
    m_vy.push_back(Acts::clampValue<float>(vertex.position4.y() /
                                           Acts::UnitConstants::mm));
    m_vz.push_back(Acts::clampValue<float>(vertex.position4.z() /
                                           Acts::UnitConstants::mm));
    m_vt.push_back(Acts::clampValue<float>(vertex.position4.w() /
                                           Acts::UnitConstants::mm));
    // TODO ingoing particles
    // outgoing particles
    std::vector<std::uint64_t> outgoing;
    for (const auto& particle : vertex.outgoing) {
      outgoing.push_back(particle.value());
    }
    m_outgoingParticles.push_back(std::move(outgoing));
    // decoded barcode components
    m_vertexPrimary.push_back(vertex.vertexId().vertexPrimary());
    m_vertexSecondary.push_back(vertex.vertexId().vertexSecondary());
    m_generation.push_back(vertex.vertexId().generation());
  }

  m_outputTree->Fill();

  m_vertexId.clear();
  m_process.clear();
  m_vx.clear();
  m_vy.clear();
  m_vz.clear();
  m_vt.clear();
  m_outgoingParticles.clear();
  m_vertexPrimary.clear();
  m_vertexSecondary.clear();
  m_generation.clear();

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
